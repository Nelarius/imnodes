#include "node_editor.h"

#include <imnodes.h>
#include <imgui.h>

#include <SDL_keycode.h>
#include <algorithm> // for std::swap
#include <cassert>
#include <chrono>
#include <cmath>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace example
{
namespace
{
template<class T>
T clamp(T x, T a, T b)
{
    return std::min(b, std::max(x, a));
}

// The type T must be POD
template<class T, size_t N>
class StaticVector
{
public:
    using Iterator = T*;
    using ConstIterator = const T*;

    StaticVector() : storage_(), size_(0) {}
    ~StaticVector() { size_ = 0; }

    // Element access

    inline T* data() { return storage_; }
    inline const T* data() const { return storage_; }

    inline T& back()
    {
        return const_cast<T&>(static_cast<const StaticVector*>(this)->back());
    }
    inline const T& back() const
    {
        assert(size_ > 0u);
        assert(size_ <= N);
        return storage_[size_ - 1];
    }

    inline T& operator[](const size_t i)
    {
        return const_cast<T&>(
            static_cast<const StaticVector*>(this)->operator[](i));
    }
    inline const T& operator[](const size_t i) const
    {
        assert(i < size_);
        return storage_[i];
    }

    inline Iterator find(const T& t)
    {
        return const_cast<Iterator>(
            static_cast<const StaticVector*>(this)->find(t));
    }
    inline ConstIterator find(const T& t) const
    {
        auto iter = begin();
        while (iter != end())
        {
            if (*iter == t)
            {
                return iter;
            }
            ++iter;
        }
        return iter;
    }

    // Capacity

    inline bool empty() const { return size_ == 0u; }

    inline size_t size() const { return size_; }

    inline size_t capacity() const { return N; }

    // Modifiers

    inline void push_back(const T& elem)
    {
        assert(size_ < N);
        storage_[size_] = elem;
        ++size_;
    }

    inline void pop_back()
    {
        assert(size_);
        --size_;
    }

    inline void swap_erase(size_t remove_at) { swap_erase(data() + remove_at); }
    inline void swap_erase(Iterator iter)
    {
        assert(size_ > 0u);
        assert(size_t(iter - begin()) < size_);

        if (iter != &back())
        {
            std::swap(*iter, back());
        }

        pop_back();
    }

    inline void clear() { size_ = 0u; }

    // Iterators

    inline Iterator begin() { return data(); }
    inline ConstIterator begin() const { return data(); }

    inline Iterator end() { return storage_ + size_; }
    inline ConstIterator end() const { return storage_ + size_; }

private:
    T storage_[N];
    size_t size_;
};

enum NodeType
{
    Node_Number,
    Node_NumberExpression, // the number isn't stored in the node, but is
                           // provided by another node
    Node_Operation,
    Node_Output
};

using StackType = std::stack<float>;
using OperationFn = void (*)(StackType&);

struct Node
{
    NodeType type;

    union
    {
        float number;
        OperationFn operation;
    };
};

struct Edge
{
    // the from, to variables store the node ids of the nodes contained in the
    // edge.
    size_t from, to;

    Edge(size_t f, size_t t) : from(f), to(t) {}

    // seems like std::unordered_map requires this to be
    // default-constructible...
    Edge() : from(), to() {}

    inline size_t opposite(size_t n) const { return n == from ? to : from; }
};

// a directional graph
class Graph
{
public:
    // the graph has a limited number of adjacencies, simplifies memory usage
    using AdjacencyArray = StaticVector<size_t, 3u>;

    using EdgeIterator = std::unordered_map<size_t, Edge>::iterator;
    using ConstEdgeIterator = std::unordered_map<size_t, Edge>::const_iterator;

    Graph()
        : current_id_(0u), nodes_(), edges_from_node_(), edges_to_node_(),
          edges_()
    {
    }

    // Element access

    inline Node& node(const size_t node_id)
    {
        return const_cast<Node&>(
            static_cast<const Graph*>(this)->node(node_id));
    }
    inline const Node& node(const size_t node_id) const
    {
        assert(nodes_.find(node_id) != nodes_.end());
        return nodes_.at(node_id);
    }

    inline const AdjacencyArray& edges_from_node(const size_t node_id)
    {
        return edges_from_node_[node_id];
    }

    inline const AdjacencyArray& edges_to_node(const size_t node_id)
    {
        return edges_to_node_[node_id];
    }

    inline Edge& edge(const size_t edge_id)
    {
        return const_cast<Edge&>(
            static_cast<const Graph*>(this)->edge(edge_id));
    }
    inline const Edge& edge(const size_t edge_id) const
    {
        assert(edges_.find(edge_id) != edges_.end());
        return edges_.at(edge_id);
    }

    inline EdgeIterator begin_edges() { return edges_.begin(); }
    inline ConstEdgeIterator end_edges() const { return edges_.begin(); }

    inline EdgeIterator end_edges() { return edges_.end(); }
    inline ConstEdgeIterator end_edge() const { return edges_.end(); }

    // Modifiers

    size_t add_node(const Node& node)
    {
        const size_t id = current_id_++;
        nodes_.insert(std::make_pair(id, node));
        edges_from_node_.insert(std::make_pair(id, AdjacencyArray()));
        edges_to_node_.insert(std::make_pair(id, AdjacencyArray()));
        return id;
    }

    void erase_node(const size_t node_id)
    {
        // first, collect all the edges from the adjacency lists
        // since erasing an edge invalidates the adjacency list iterators
        StaticVector<size_t, 6> edges_to_erase;
        for (size_t edge : edges_from_node_[node_id])
        {
            edges_to_erase.push_back(edge);
        }
        for (size_t edge : edges_to_node_[node_id])
        {
            edges_to_erase.push_back(edge);
        }
        for (size_t edge : edges_to_erase)
        {
            erase_edge(edge);
        }
        nodes_.erase(node_id);
        edges_from_node_.erase(node_id);
        edges_to_node_.erase(node_id);
    }

    size_t add_edge(const size_t from, const size_t to)
    {
        const size_t id = current_id_++;
        edges_.insert(std::make_pair(id, Edge(from, to)));
        edges_from_node_[from].push_back(id);
        edges_to_node_[to].push_back(id);
        return id;
    }

    void erase_edge(const size_t edge_id)
    {
        auto edge = edges_.find(edge_id);
        assert(edge != edges_.end());

        {
            auto& edges_from = edges_from_node_[edge->second.from];
            auto iter = edges_from.find(edge_id);
            assert(iter != edges_from.end());
            edges_from.swap_erase(iter);
        }

        {
            auto& edges_to = edges_to_node_[edge->second.to];
            auto iter = edges_to.find(edge_id);
            assert(iter != edges_to.end());
            edges_to.swap_erase(iter);
        }

        edges_.erase(edge);
    }

    ImU32 evaluate(const size_t root_node)
    {
        // this function does a depth-first evaluation of the graph
        // the nodes are evaluated post-order using two stacks.
        std::stack<float> eval_stack;
        std::stack<size_t> preorder;
        std::stack<size_t> postorder;

        preorder.push(root_node);

        while (!preorder.empty())
        {
            const size_t node = preorder.top();
            preorder.pop();

            postorder.push(node);

            for (const size_t edge : edges_from_node_[node])
            {
                const size_t neighbor = edges_[edge].opposite(node);
                assert(neighbor != root_node);
                preorder.push(neighbor);
            }
        }

        // unwind the stack and call each operation along the way
        while (!postorder.empty())
        {
            const size_t node = postorder.top();
            postorder.pop();
            switch (nodes_[node].type)
            {
            case Node_Number:
                eval_stack.push(nodes_[node].number);
                break;
            case Node_NumberExpression:
                break;
            case Node_Operation:
                nodes_[node].operation(eval_stack);
                break;
            case Node_Output:
                break;
            default:
                assert("Invalid enum value!");
            }
        }

        // there should be three values on the stack
        assert(eval_stack.size() == 3);
        const int b = 255 * clamp(eval_stack.top(), 0.f, 1.f);
        eval_stack.pop();
        const int g = 255 * clamp(eval_stack.top(), 0.f, 1.f);
        eval_stack.pop();
        const int r = 255 * clamp(eval_stack.top(), 0.f, 1.f);
        eval_stack.pop();

        return IM_COL32(r, g, b, 255);
    }

private:
    size_t current_id_;
    std::unordered_map<size_t, Node> nodes_;
    std::unordered_map<size_t, AdjacencyArray> edges_from_node_;
    std::unordered_map<size_t, AdjacencyArray> edges_to_node_;
    std::unordered_map<size_t, Edge> edges_;
};

struct TimeContext
{
    std::chrono::steady_clock::time_point start;
    float seconds_elapsed;

    TimeContext()
        : start(std::chrono::steady_clock::now()), seconds_elapsed(0.f)
    {
    }

    void update()
    {
        seconds_elapsed = std::chrono::duration_cast<
                              std::chrono::duration<float, std::ratio<1>>>(
                              std::chrono::steady_clock::now() - start)
                              .count();
    }
};

TimeContext time_context;

void operation_time(std::stack<float>& stack)
{
    stack.push(time_context.seconds_elapsed);
}

void operation_sine(std::stack<float>& stack)
{
    const float x = stack.top();
    stack.pop();
    stack.push(std::abs(std::sin(x)));
}

void operation_multiply(std::stack<float>& stack)
{
    const float rhs = stack.top();
    stack.pop();
    const float lhs = stack.top();
    stack.pop();
    stack.push(lhs * rhs);
}

void operation_add(std::stack<float>& stack)
{
    const float rhs = stack.top();
    stack.pop();
    const float lhs = stack.top();
    stack.pop();
    stack.push(lhs + rhs);
}

class ColorNodeEditor
{
public:
    ColorNodeEditor() = default;
    ~ColorNodeEditor() = default;

    void show()
    {
        time_context.update();

        ImGui::Begin("Color node editor");
        ImGui::Columns(2);
        ImGui::TextUnformatted("A -- add node");
        ImGui::TextUnformatted("X -- deleted selected node or link");
        ImGui::NextColumn();
        ImGui::Checkbox(
            "emulate three button mouse",
            &imnodes::GetIO().emulate_three_button_mouse.enabled);
        ImGui::Columns(1);

        imnodes::PushAttributeFlag(
            imnodes::AttributeFlags_EnableLinkDetachWithDragClick);
        imnodes::BeginNodeEditor();

        for (const auto& node : output_nodes_)
        {
            const float node_width = 100.0f;
            imnodes::PushColorStyle(
                imnodes::ColorStyle_TitleBar, IM_COL32(11, 109, 191, 255));
            imnodes::PushColorStyle(
                imnodes::ColorStyle_TitleBarHovered,
                IM_COL32(45, 126, 194, 255));
            imnodes::PushColorStyle(
                imnodes::ColorStyle_TitleBarSelected,
                IM_COL32(81, 148, 204, 255));
            imnodes::BeginNode(node.out);

            imnodes::BeginNodeTitleBar();
            ImGui::TextUnformatted("output");
            imnodes::EndNodeTitleBar();

            ImGui::Dummy(ImVec2(node_width, 0.f));
            {
                // TODO: the color style of the pin needs to be pushed here
                imnodes::BeginInputAttribute(int(node.red));
                const float label_width = ImGui::CalcTextSize("r").x;
                ImGui::Text("r");
                if (graph_.node(node.red).type == Node_Number)
                {
                    ImGui::SameLine();
                    ImGui::PushItemWidth(node_width - label_width);
                    ImGui::DragFloat(
                        "##hidelabel",
                        &graph_.node(node.red).number,
                        0.01f,
                        0.f,
                        1.0f);
                    ImGui::PopItemWidth();
                }
                imnodes::EndAttribute();
            }

            ImGui::Spacing();

            {
                imnodes::BeginInputAttribute(int(node.green));
                const float label_width = ImGui::CalcTextSize("g").x;
                ImGui::Text("g");
                if (graph_.node(node.green).type == Node_Number)
                {
                    ImGui::SameLine();
                    ImGui::PushItemWidth(node_width - label_width);
                    ImGui::DragFloat(
                        "##hidelabel",
                        &graph_.node(node.green).number,
                        0.01f,
                        0.f,
                        1.f);
                    ImGui::PopItemWidth();
                }
                imnodes::EndAttribute();
            }

            ImGui::Spacing();

            {
                imnodes::BeginInputAttribute(int(node.blue));
                const float label_width = ImGui::CalcTextSize("b").x;
                ImGui::Text("b");
                if (graph_.node(node.blue).type == Node_Number)
                {
                    ImGui::SameLine();
                    ImGui::PushItemWidth(node_width - label_width);
                    ImGui::DragFloat(
                        "##hidelabel",
                        &graph_.node(node.blue).number,
                        0.01f,
                        0.f,
                        1.0f);
                    ImGui::PopItemWidth();
                }
                imnodes::EndAttribute();
            }
            imnodes::EndNode();
            imnodes::PopColorStyle();
            imnodes::PopColorStyle();
            imnodes::PopColorStyle();
        }

        for (const auto& node : sine_nodes_)
        {
            const float node_width = 100.0f;
            imnodes::BeginNode(node.op);

            imnodes::BeginNodeTitleBar();
            ImGui::TextUnformatted("sine");
            imnodes::EndNodeTitleBar();

            {
                imnodes::BeginInputAttribute(int(node.input));
                const float label_width = ImGui::CalcTextSize("number").x;
                ImGui::Text("number");
                if (graph_.node(node.input).type == Node_Number)
                {
                    ImGui::SameLine();
                    ImGui::PushItemWidth(node_width - label_width);
                    ImGui::DragFloat(
                        "##hidelabel",
                        &graph_.node(node.input).number,
                        0.01f,
                        0.f,
                        1.0f);
                    ImGui::PopItemWidth();
                }
                imnodes::EndAttribute();
            }

            ImGui::Spacing();

            {
                imnodes::BeginOutputAttribute(int(node.op));
                const float label_width = ImGui::CalcTextSize("output").x;
                ImGui::Indent(node_width - label_width);
                ImGui::Text("output");
                imnodes::EndAttribute();
            }

            imnodes::EndNode();
        }

        for (const auto& node : time_nodes_)
        {
            imnodes::BeginNode(node);

            imnodes::BeginNodeTitleBar();
            ImGui::TextUnformatted("time");
            imnodes::EndNodeTitleBar();

            imnodes::BeginOutputAttribute(int(node));
            ImGui::Text("output");
            imnodes::EndAttribute();

            imnodes::EndNode();
        }

        for (const auto& node : mul_nodes_)
        {
            const float node_width = 100.0f;
            imnodes::BeginNode(node.op);

            imnodes::BeginNodeTitleBar();
            ImGui::TextUnformatted("multiply");
            imnodes::EndNodeTitleBar();

            {
                imnodes::BeginInputAttribute(int(node.lhs));
                const float label_width = ImGui::CalcTextSize("left").x;
                ImGui::Text("left");
                if (graph_.node(node.lhs).type == Node_Number)
                {
                    ImGui::SameLine();
                    ImGui::PushItemWidth(node_width - label_width);
                    ImGui::DragFloat(
                        "##hidelabel", &graph_.node(node.lhs).number, 0.01f);
                    ImGui::PopItemWidth();
                }
                imnodes::EndAttribute();
            }

            {
                imnodes::BeginInputAttribute(int(node.rhs));
                const float label_width = ImGui::CalcTextSize("right").x;
                ImGui::Text("right");
                if (graph_.node(node.rhs).type == Node_Number)
                {
                    ImGui::SameLine();
                    ImGui::PushItemWidth(node_width - label_width);
                    ImGui::DragFloat(
                        "##hidelabel", &graph_.node(node.rhs).number, 0.01f);
                    ImGui::PopItemWidth();
                }
                imnodes::EndAttribute();
            }

            ImGui::Spacing();

            {
                imnodes::BeginOutputAttribute(int(node.op));
                const float label_width = ImGui::CalcTextSize("result").x;
                ImGui::Indent(node_width - label_width);
                ImGui::Text("result");
                imnodes::EndAttribute();
            }

            imnodes::EndNode();
        }

        for (const auto& node : add_nodes_)
        {
            const float node_width = 100.0f;
            imnodes::BeginNode(node.op);

            imnodes::BeginNodeTitleBar();
            ImGui::TextUnformatted("add");
            imnodes::EndNodeTitleBar();

            {
                imnodes::BeginInputAttribute(int(node.lhs));
                const float label_width = ImGui::CalcTextSize("left").x;
                ImGui::Text("left");
                if (graph_.node(node.lhs).type == Node_Number)
                {
                    ImGui::SameLine();
                    ImGui::PushItemWidth(node_width - label_width);
                    ImGui::DragFloat(
                        "##hidelabel", &graph_.node(node.lhs).number, 0.01f);
                    ImGui::PopItemWidth();
                }
                imnodes::EndAttribute();
            }

            {
                imnodes::BeginInputAttribute(int(node.rhs));
                const float label_width = ImGui::CalcTextSize("right").x;
                ImGui::Text("right");
                if (graph_.node(node.rhs).type == Node_Number)
                {
                    ImGui::SameLine();
                    ImGui::PushItemWidth(node_width - label_width);
                    ImGui::DragFloat(
                        "##hidelabel", &graph_.node(node.rhs).number, 0.01f);
                    ImGui::PopItemWidth();
                }
                imnodes::EndAttribute();
            }

            ImGui::Spacing();

            {
                imnodes::BeginOutputAttribute(int(node.op));
                const float label_width = ImGui::CalcTextSize("result").x;
                ImGui::Indent(node_width - label_width);
                ImGui::Text("result");
                imnodes::EndAttribute();
            }

            imnodes::EndNode();
        }

        for (auto iter = graph_.begin_edges(); iter != graph_.end_edges();
             ++iter)
        {
            // don't render internal edges
            // internal edges always look like
            //
            // Node_Output | Node_Operation
            // ->
            // Node_Number | Node_NumberExpression
            const NodeType type = graph_.node(iter->second.to).type;
            if (type == Node_Number || type == Node_NumberExpression)
                continue;
            imnodes::Link(iter->first, iter->second.from, iter->second.to);
        }

        const bool open_popup =
            ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            imnodes::IsEditorHovered() && ImGui::IsKeyReleased(SDL_SCANCODE_A);

        imnodes::EndNodeEditor();
        imnodes::PopAttributeFlag();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
        if (!ImGui::IsAnyItemHovered() && open_popup)
        {
            ImGui::OpenPopup("add node");
        }

        if (ImGui::BeginPopup("add node"))
        {
            ImVec2 click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();

            if (ImGui::MenuItem("output") && output_nodes_.size() == 0u)
            {
                // The output node takes three input attributes:
                // r, g, b
                // They are rendered in top-to-bottom order in the node UI
                // By convention, the adjacencies are added in top-to-bottom
                // order
                OutputNode node;
                Node num = Node{Node_Number, 0.f};
                Node out = Node{Node_Output, 0.f};

                node.red = graph_.add_node(num);
                node.green = graph_.add_node(num);
                node.blue = graph_.add_node(num);
                node.out = graph_.add_node(out);

                output_nodes_.push_back(node);

                graph_.add_edge(node.out, node.red);
                graph_.add_edge(node.out, node.green);
                graph_.add_edge(node.out, node.blue);

                imnodes::SetNodeScreenSpacePos(node.out, click_pos);
            }

            if (ImGui::MenuItem("time"))
            {
                Node op = Node{Node_Operation, 0.f};
                op.operation = operation_time;

                const size_t node = graph_.add_node(op);
                time_nodes_.push_back(node);

                imnodes::SetNodeScreenSpacePos(node, click_pos);
            }

            if (ImGui::MenuItem("sine"))
            {
                SineNode node;

                Node num{Node_Number, 0.f};
                Node op{Node_Operation, 0.f};
                op.operation = operation_sine;

                node.input = graph_.add_node(num);
                node.op = graph_.add_node(op);

                graph_.add_edge(node.op, node.input);

                sine_nodes_.push_back(node);

                imnodes::SetNodeScreenSpacePos(node.op, click_pos);
            }

            if (ImGui::MenuItem("multiply"))
            {
                MultiplyNode node;

                Node num{Node_Number, 0.f};
                Node op{Node_Operation, 0.f};
                op.operation = operation_multiply;

                node.lhs = graph_.add_node(num);
                node.rhs = graph_.add_node(num);
                node.op = graph_.add_node(op);

                graph_.add_edge(node.op, node.lhs);
                graph_.add_edge(node.op, node.rhs);

                mul_nodes_.push_back(node);

                imnodes::SetNodeScreenSpacePos(node.op, click_pos);
            }

            if (ImGui::MenuItem("add"))
            {
                AddNode node;

                Node num{Node_Number, 0.f};
                Node op{Node_Operation, 0.f};
                op.operation = operation_add;

                node.lhs = graph_.add_node(num);
                node.rhs = graph_.add_node(num);
                node.op = graph_.add_node(op);

                graph_.add_edge(node.op, node.lhs);
                graph_.add_edge(node.op, node.rhs);

                add_nodes_.push_back(node);

                imnodes::SetNodeScreenSpacePos(node.op, click_pos);
            }
            ImGui::EndPopup();
        }

        ImGui::PopStyleVar();

        {
            const int num_selected = imnodes::NumSelectedLinks();
            if (num_selected > 0 && ImGui::IsKeyReleased(SDL_SCANCODE_X))
            {
                static std::vector<int> selected_links;
                selected_links.resize(static_cast<size_t>(num_selected), -1);
                imnodes::GetSelectedLinks(selected_links.data());
                for (const int link_id : selected_links)
                {
                    assert(link_id >= 0);
                    Node& node = graph_.node(graph_.edge(link_id).from);
                    assert(node.type == Node_NumberExpression);
                    node.type = Node_Number;
                    graph_.erase_edge(size_t(link_id));
                }
                selected_links.clear();
            }
        }

        {
            const int num_selected = imnodes::NumSelectedNodes();
            if (num_selected > 0 && ImGui::IsKeyReleased(SDL_SCANCODE_X))
            {
                static std::vector<int> selected_nodes;
                selected_nodes.resize(static_cast<size_t>(num_selected), -1);
                imnodes::GetSelectedNodes(selected_nodes.data());
                for (const int node_id : selected_nodes)
                {
                    find_and_remove_node(node_id);
                }
                selected_nodes.clear();
            }
        }

        Id link_start, link_end;
        if (imnodes::IsLinkCreated(&link_start.id, &link_end.id))
        {
            // in the expression graph, we want the edge to always go from
            // the number to the operation, since the graph is directed!

            const Node& from_node = graph_.node(link_start);
            const Node& to_node = graph_.node(link_end);

            const bool invalid_node = from_node.type == to_node.type;

            if (!invalid_node)
            {
                if (from_node.type == Node_Operation)
                {
                    std::swap(link_start, link_end);
                }

                graph_.add_edge(
                    static_cast<size_t>(link_start.id),
                    static_cast<size_t>(link_end.id));

                Node& node_from = graph_.node(link_start);
                node_from.type = node_from.type == Node_Number
                                     ? Node_NumberExpression
                                     : node_from.type;
            }
        }

        Id link_id;
        if (imnodes::IsLinkDestroyed(&link_id.id))
        {
            graph_.erase_edge(link_id);
        }

        ImGui::End();

        ImU32 color = IM_COL32(255, 20, 147, 255);
        if (output_nodes_.size() > 0u)
        {
            color = graph_.evaluate(output_nodes_[0u].out);
        }

        ImGui::PushStyleColor(ImGuiCol_WindowBg, color);
        ImGui::Begin("output color");
        ImGui::End();
        ImGui::PopStyleColor();
    }

private:
    struct OutputNode
    {
        size_t red, green, blue, out;
    };

    struct SineNode
    {
        size_t input, op;
    };

    struct MultiplyNode
    {
        size_t lhs, rhs, op;
    };

    struct AddNode
    {
        size_t lhs, rhs, op;
    };

    struct Id
    {
        int id;

        inline bool is_valid() const { return id >= 0; }

        inline void invalidate() { id = invalid_index; }

        inline operator int() const
        {
            assert(is_valid());
            return id;
        }

        inline Id& operator=(int i)
        {
            id = i;
            return *this;
        }

        inline bool operator==(int i) const { return id == i; }

        Id() : id(invalid_index) {}

    private:
        static const int invalid_index = -1;
    };

    inline void find_and_remove_node(const int id)
    {
        // this function is a spectacular feat of engineering...

        assert(id >= 0);

        if (remove_output_node(id))
            return;

        if (remove_time_node(id))
            return;

        if (remove_sine_node(id))
            return;

        if (remove_mul_node(id))
            return;

        remove_add_node(id);
    }

    inline bool remove_output_node(const int id)
    {
        if (output_nodes_.size() > 0u && output_nodes_[0].out == id)
        {
            const auto& node = output_nodes_[0];
            graph_.erase_node(node.out);
            graph_.erase_node(node.red);
            graph_.erase_node(node.green);
            graph_.erase_node(node.blue);
            output_nodes_.pop_back();
            return true;
        }
        return false;
    }

    inline bool remove_time_node(const int id)
    {
        auto iter =
            std::find(time_nodes_.begin(), time_nodes_.end(), size_t(id));
        if (iter != time_nodes_.end())
        {
            graph_.erase_node(*iter);
            time_nodes_.erase(iter);
            return true;
        }
        return false;
    }

    inline bool remove_sine_node(const int id)
    {
        for (auto iter = sine_nodes_.begin(); iter != sine_nodes_.end(); ++iter)
        {
            if (iter->op == id)
            {
                graph_.erase_node(iter->op);
                graph_.erase_node(iter->input);
                sine_nodes_.erase(iter);
                return true;
            }
        }
        return false;
    }

    inline bool remove_mul_node(const int id)
    {
        for (auto iter = mul_nodes_.begin(); iter != mul_nodes_.end(); ++iter)
        {
            if (iter->op == id)
            {
                graph_.erase_node(iter->op);
                graph_.erase_node(iter->lhs);
                graph_.erase_node(iter->rhs);
                mul_nodes_.erase(iter);
                return true;
            }
        }
        return false;
    }

    inline bool remove_add_node(const int id)
    {
        for (auto iter = add_nodes_.begin(); iter != add_nodes_.end(); ++iter)
        {
            if (iter->op == id)
            {
                graph_.erase_node(iter->op);
                graph_.erase_node(iter->lhs);
                graph_.erase_node(iter->rhs);
                add_nodes_.erase(iter);
                return true;
            }
        }
        return false;
    }

    Graph graph_;
    StaticVector<OutputNode, 1> output_nodes_;
    std::vector<size_t>
        time_nodes_; // just a single node representing the operation
    std::vector<SineNode> sine_nodes_;
    std::vector<MultiplyNode> mul_nodes_;
    std::vector<AddNode> add_nodes_;
}; // namespace

static ColorNodeEditor color_editor;
} // namespace

void NodeEditorInitialize()
{
    auto& style = imnodes::GetStyle();
    style.colors[imnodes::ColorStyle_TitleBar] = IM_COL32(232, 27, 86, 255);
    style.colors[imnodes::ColorStyle_TitleBarHovered] =
        IM_COL32(235, 67, 115, 255);
    style.colors[imnodes::ColorStyle_TitleBarSelected] =
        IM_COL32(241, 108, 146, 255);
    style.colors[imnodes::ColorStyle_Link] = IM_COL32(255, 210, 0, 255);
    style.colors[imnodes::ColorStyle_LinkHovered] =
        IM_COL32(255, 229, 108, 255);
    style.colors[imnodes::ColorStyle_LinkSelected] =
        IM_COL32(255, 237, 154, 255);
    style.colors[imnodes::ColorStyle_Pin] = IM_COL32(255, 210, 0, 255);
    style.colors[imnodes::ColorStyle_PinHovered] = IM_COL32(255, 237, 154, 255);
    style.flags = imnodes::StyleFlags_GridLines;
}

void NodeEditorShow() { color_editor.show(); }

void NodeEditorShutdown() {}
} // namespace example
