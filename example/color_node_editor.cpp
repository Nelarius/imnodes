#include "imgui.h"
#include "imnodes.h"
#include "node_editor.h"

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
// The type T must be POD
template<class T, size_t N>
class StaticArray
{
public:
    using Iterator = T*;
    using ConstIterator = const T*;

    StaticArray() : storage_(), size_(0) {}
    ~StaticArray() { size_ = 0; }

    // Element access

    inline T* data() { return storage_; }
    inline const T* data() const { return storage_; }

    inline T& back()
    {
        return const_cast<T&>(static_cast<const StaticArray*>(this)->back());
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
            static_cast<const StaticArray*>(this)->operator[](i));
    }
    inline const T& operator[](const size_t i) const
    {
        assert(i < size_);
        return storage_[i];
    }

    inline Iterator find(const T& t)
    {
        return const_cast<Iterator>(
            static_cast<const StaticArray*>(this)->find(t));
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

    inline void swap_erase(size_t remove_at)
    {
        swap_remove(data() + remove_at);
    }
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

class Graph
{
public:
    // the graph has a limited number of adjacencies, simplifies memory usage
    using AdjacencyArray = StaticArray<size_t, 6u>;

    using EdgeIterator = std::unordered_map<size_t, Edge>::iterator;
    using ConstEdgeIterator = std::unordered_map<size_t, Edge>::const_iterator;

    Graph() : current_id_(0u), nodes_(), edge_adjacencies_(), edges_() {}

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

    // Capacity

    inline size_t num_adjacencies(const size_t node_id) const
    {
        const auto iter = edge_adjacencies_.find(node_id);
        assert(iter != edge_adjacencies_.end());
        return iter->second.size();
    }

    // Modifiers

    size_t add_node(const Node& node)
    {
        const size_t id = current_id_++;
        nodes_.insert(std::make_pair(id, node));
        edge_adjacencies_.insert(std::make_pair(id, AdjacencyArray()));
        return id;
    }

    void erase_node(const size_t node_id)
    {
        for (size_t edge : edge_adjacencies_[node_id])
        {
            erase_edge(edge);
        }
        nodes_.erase(node_id);
        edge_adjacencies_.erase(node_id);
    }

    size_t add_edge(const size_t from, const size_t to)
    {
        const size_t id = current_id_++;
        edges_.insert(std::make_pair(id, Edge(from, to)));
        edge_adjacencies_[from].push_back(id);
        edge_adjacencies_[to].push_back(id);
        return id;
    }

    void erase_edge(const size_t edge_id)
    {
        auto edge = edges_.find(edge_id);

        {
            auto& adjacencies = edge_adjacencies_[edge->second.from];
            auto iter = adjacencies.find(edge_id);
            assert(iter != adjacencies.end());
            adjacencies.swap_erase(iter);
        }

        {
            auto& adjacencies = edge_adjacencies_[edge->second.to];
            auto iter = adjacencies.find(edge_id);
            assert(iter != adjacencies.end());
            adjacencies.swap_erase(iter);
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
        std::unordered_set<size_t> visited;

        preorder.push(root_node);

        while (!preorder.empty())
        {
            const size_t node = preorder.top();
            preorder.pop();
            postorder.push(node);
            visited.insert(node);

            for (const size_t edge : edge_adjacencies_[node])
            {
                const size_t neighbor = edges_[edge].opposite(node);
                if (visited.find(neighbor) != visited.end())
                    continue;

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
        const int b = 255 * eval_stack.top();
        eval_stack.pop();
        const int g = 255 * eval_stack.top();
        eval_stack.pop();
        const int r = 255 * eval_stack.top();
        eval_stack.pop();

        return IM_COL32(r, g, b, 255);
    }

private:
    size_t current_id_;
    std::unordered_map<size_t, Node> nodes_;
    // the edges which each node is adjacent to -- the adjacency array
    // stores edge ids
    std::unordered_map<size_t, AdjacencyArray> edge_adjacencies_;
    std::unordered_map<size_t, Edge> edges_;
};

struct TimeContext
{
    std::chrono::steady_clock::time_point start;

    TimeContext() : start(std::chrono::steady_clock::now()) {}
};

TimeContext time_context;

void operation_time(std::stack<float>& stack)
{
    const float seconds =
        std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1>>>(
            std::chrono::steady_clock::now() - time_context.start)
            .count();
    stack.push(seconds);
}

void operation_sine(std::stack<float>& stack)
{
    const float x = stack.top();
    stack.pop();
    stack.push(std::sin(x));
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
        ImGui::Begin("Color node editor");
        imnodes::BeginNodeEditor();

        for (const auto& node : output_nodes_)
        {
            const float node_width = 100.0f;
            imnodes::BeginNode(node.out);
            imnodes::Name("output");

            ImGui::Dummy(ImVec2(node_width, 0.f));
            {
                imnodes::BeginAttribute(
                    int(node.red), imnodes::AttributeType_Input);
                const float label_width = ImGui::CalcTextSize("r").x;
                ImGui::Text("r");
                ImGui::SameLine();
                ImGui::PushItemWidth(node_width - label_width);
                ImGui::DragFloat(
                    "##hidelabel",
                    &graph_.node(node.red).number,
                    0.01f,
                    0.f,
                    1.0f);
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }

            ImGui::Spacing();

            {
                imnodes::BeginAttribute(
                    int(node.green), imnodes::AttributeType_Input);
                const float label_width = ImGui::CalcTextSize("g").x;
                ImGui::Text("g");
                ImGui::SameLine();
                ImGui::PushItemWidth(node_width - label_width);
                ImGui::DragFloat(
                    "##hidelabel",
                    &graph_.node(node.green).number,
                    0.01f,
                    0.f,
                    1.f);
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }

            ImGui::Spacing();

            {
                imnodes::BeginAttribute(
                    int(node.blue), imnodes::AttributeType_Input);
                const float label_width = ImGui::CalcTextSize("b").x;
                ImGui::Text("b");
                ImGui::SameLine();
                ImGui::PushItemWidth(node_width - label_width);
                ImGui::DragFloat(
                    "##hidelabel",
                    &graph_.node(node.blue).number,
                    0.01f,
                    0.f,
                    1.0f);
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }
            imnodes::EndNode();
        }

        for (const auto& node : sine_nodes_)
        {
            const float node_width = 100.0f;
            imnodes::BeginNode(node.op);
            imnodes::Name("sine");

            {
                imnodes::BeginAttribute(
                    int(node.input), imnodes::AttributeType_Input);
                const float label_width = ImGui::CalcTextSize("number").x;
                ImGui::Text("number");
                ImGui::SameLine();
                ImGui::PushItemWidth(node_width - label_width);
                ImGui::DragFloat(
                    "##hidelabel",
                    &graph_.node(node.input).number,
                    0.01f,
                    0.f,
                    1.0f);
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }

            ImGui::Spacing();

            {
                imnodes::BeginAttribute(
                    int(node.op), imnodes::AttributeType_Output);
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
            imnodes::Name("time");

            imnodes::BeginAttribute(int(node), imnodes::AttributeType_Output);
            ImGui::Text("output");
            imnodes::EndAttribute();

            imnodes::EndNode();
        }

        for (const auto& node : mul_nodes_)
        {
            const float node_width = 100.0f;
            imnodes::BeginNode(node.op);
            imnodes::Name("multiply");

            {
                imnodes::BeginAttribute(
                    int(node.lhs), imnodes::AttributeType_Input);
                const float label_width = ImGui::CalcTextSize("left").x;
                ImGui::Text("left");
                ImGui::SameLine();
                ImGui::PushItemWidth(node_width - label_width);
                ImGui::DragFloat(
                    "##hidelabel", &graph_.node(node.lhs).number, 0.01f);
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }

            {
                imnodes::BeginAttribute(
                    int(node.rhs), imnodes::AttributeType_Input);
                const float label_width = ImGui::CalcTextSize("right").x;
                ImGui::Text("right");
                ImGui::SameLine();
                ImGui::PushItemWidth(node_width - label_width);
                ImGui::DragFloat(
                    "##hidelabel", &graph_.node(node.rhs).number, 0.01f);
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }

            ImGui::Spacing();

            {
                imnodes::BeginAttribute(
                    int(node.op), imnodes::AttributeType_Output);
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
            imnodes::Name("add");

            {
                imnodes::BeginAttribute(
                    int(node.lhs), imnodes::AttributeType_Input);
                const float label_width = ImGui::CalcTextSize("left").x;
                ImGui::Text("left");
                ImGui::SameLine();
                ImGui::PushItemWidth(node_width - label_width);
                ImGui::DragFloat(
                    "##hidelabel", &graph_.node(node.lhs).number, 0.01f);
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }

            {
                imnodes::BeginAttribute(
                    int(node.rhs), imnodes::AttributeType_Input);
                const float label_width = ImGui::CalcTextSize("right").x;
                ImGui::Text("right");
                ImGui::SameLine();
                ImGui::PushItemWidth(node_width - label_width);
                ImGui::DragFloat(
                    "##hidelabel", &graph_.node(node.rhs).number, 0.01f);
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }

            ImGui::Spacing();

            {
                imnodes::BeginAttribute(
                    int(node.op), imnodes::AttributeType_Output);
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
            const NodeType type = graph_.node(iter->second.to).type;
            if (type == Node_Operation || type == Node_Output)
                continue;
            imnodes::Link(iter->second.from, iter->second.to);
        }

        const bool open_popup =
            ImGui::IsMouseClicked(1) || ImGui::IsKeyReleased(SDL_SCANCODE_A);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
        if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseHoveringWindow() &&
            open_popup)
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

                graph_.add_edge(node.red, node.out);
                graph_.add_edge(node.green, node.out);
                graph_.add_edge(node.blue, node.out);

                imnodes::SetNodePos(node.out, click_pos, ImGuiCond_Appearing);
            }

            if (ImGui::MenuItem("time"))
            {
                Node op = Node{Node_Operation, 0.f};
                op.operation = operation_time;

                const size_t node = graph_.add_node(op);
                time_nodes_.push_back(node);

                imnodes::SetNodePos(node, click_pos, ImGuiCond_Appearing);
            }

            if (ImGui::MenuItem("sine"))
            {
                SineNode node;

                Node num{Node_Number, 0.f};
                Node op{Node_Operation, 0.f};
                op.operation = operation_sine;

                node.input = graph_.add_node(num);
                node.op = graph_.add_node(op);

                sine_nodes_.push_back(node);

                imnodes::SetNodePos(node.op, click_pos, ImGuiCond_Appearing);
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

                graph_.add_edge(node.lhs, node.op);
                graph_.add_edge(node.rhs, node.op);

                mul_nodes_.push_back(node);

                imnodes::SetNodePos(node.op, click_pos, ImGuiCond_Appearing);
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

                graph_.add_edge(node.lhs, node.op);
                graph_.add_edge(node.rhs, node.op);

                add_nodes_.push_back(node);

                imnodes::SetNodePos(node.op, click_pos, ImGuiCond_Appearing);
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();

        imnodes::EndNodeEditor();

        // TODO: Handle events here
        ui_.reset();

        imnodes::IsPinHovered(&ui_.pin_hovered.index);

        imnodes::IsNodeHovered(&ui_.node_selected.index);

        if (imnodes::IsLinkSelected(&ui_.link_start.index, &ui_.link_end.index))
        {
            // TODO: is this needed? why not just handle the event here
            ui_.is_link_selected = true;
        }

        imnodes::IsNodeSelected(&ui_.node_selected.index);

        if (ImGui::IsKeyReleased(SDL_SCANCODE_X))
        {
            if (ui_.is_link_selected)
            {
                // TODO
            }

            if (ui_.node_selected.is_valid())
            {
                if (output_nodes_.size() > 0 &&
                    output_nodes_[0].out == ui_.node_selected.index)
                {
                    const auto& node = output_nodes_[0];
                    graph_.erase_node(node.red);
                    graph_.erase_node(node.green);
                    graph_.erase_node(node.blue);
                    graph_.erase_node(node.out);
                    output_nodes_.pop_back();
                }
            }
        }

        // Now Ui::link_start and Ui::link_end can be re-used

        if (imnodes::IsLinkCreated(&ui_.link_start.index, &ui_.link_end.index))
        {
            graph_.add_edge(ui_.link_start, ui_.link_end);
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

    struct Index
    {
        int index;

        inline bool is_valid() const { return index >= 0; }

        inline void invalidate() { index = invalid_index; }

        inline operator int() const
        {
            assert(is_valid());
            return index;
        }

        inline Index& operator=(int i)
        {
            index = i;
            return *this;
        }

        inline bool operator==(int i) const { return index == i; }

        Index() : index(invalid_index) {}

    private:
        static const int invalid_index = -1;
    };

    struct Ui
    {
        Index pin_hovered;
        Index node_selected;
        bool is_link_selected;
        bool is_link_created;
        Index link_start, link_end;

        Ui()
            : pin_hovered(), node_selected(), is_link_selected(false),
              link_start(), link_end()
        {
        }

        inline void reset()
        {
            // reset frame data
            pin_hovered.invalidate();
            node_selected.invalidate();
            is_link_selected = false;
            link_start.invalidate();
            link_end.invalidate();
        }
    };

    Graph graph_;
    StaticArray<OutputNode, 1> output_nodes_;
    std::vector<size_t>
        time_nodes_; // just a single node representing the operation
    std::vector<SineNode> sine_nodes_;
    std::vector<MultiplyNode> mul_nodes_;
    std::vector<AddNode> add_nodes_;

    Ui ui_;
}; // namespace

static ColorNodeEditor color_editor;
} // namespace

void NodeEditorInitialize() {}

void NodeEditorShow() { color_editor.show(); }

void NodeEditorShutdown() {}
} // namespace example
