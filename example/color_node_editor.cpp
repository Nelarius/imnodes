#include "imgui.h"
#include "imnodes.h"
#include "node_editor.h"

#include <algorithm> // for std::swap
#include <cassert>
#include <stack>
#include <unordered_set>
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
        return *(storage_ + (size_ - 1));
    }

    inline T& operator[](const size_t i)
    {
        return const_cast<T&>(
            static_cast<const StaticArray*>(this)->operator[](i));
    }
    inline const T& operator[](const size_t i) const
    {
        assert(i < size_);
        return *(data() + size_);
    }

    // Capacity

    inline bool empty() const { return size_ == 0u; }

    inline size_t size() const { return size_; }

    inline size_t capacity() const { return N; }

    // Modifiers

    inline void push_back(const T& elem)
    {
        assert(size_ < N);
        data()[size_] = elem;
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

class Graph
{
public:
    // the graph has a limited number of adjacencies, simplifies memory usage
    using AdjacencyArray = StaticArray<size_t, 3u>;
    using AdjacencyIterator = AdjacencyArray::Iterator;
    using ConstAdjacencyIterator = AdjacencyArray::ConstIterator;
    using NodeIterator = std::vector<Node>::iterator;
    using ConstNodeIterator = std::vector<Node>::const_iterator;

    Graph() = default;

    // Element access

    inline Node& operator[](const size_t i)
    {
        return const_cast<Node&>(
            static_cast<const Graph*>(this)->operator[](i));
    }
    inline const Node& operator[](const size_t i) const
    {
        assert(i < nodes_.size());
        return nodes_[i];
    }

    // Capacity

    inline size_t num_adjacencies(const size_t node) const
    {
        assert(node < nodes_.size());
        return adjacencies_[node].size();
    }

    // Modifiers

    size_t add_node(const Node& node)
    {
        size_t handle = nodes_.size();
        nodes_.push_back(node);
        adjacencies_.push_back(AdjacencyArray());
        return handle;
    }

    void add_edge(size_t node1, size_t node2)
    {
        adjacencies_[node1].push_back(node2);
        adjacencies_[node2].push_back(node1);
    }

    void erase_edge(size_t node1, size_t node2)
    {
        remove_adjacency(node1, node2);
        remove_adjacency(node2, node1);
    }

    void clear()
    {
        nodes_.clear();

        for (auto& adjacency : adjacencies_)
        {
            adjacency.clear();
        }
        adjacencies_.clear();
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

            for (const size_t neighbor : adjacencies_[node])
            {
                if (visited.find(neighbor) != visited.end())
                    continue;

                preorder.push(neighbor);
            }
        }

        while (!postorder.empty())
        {
            const size_t node = postorder.top();
            postorder.pop();
            // unwinding the stack
            switch (nodes_[node].type)
            {
                case Node_Number:
                    eval_stack.push(nodes_[node].number);
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
    inline void remove_adjacency(size_t at_node, size_t index)
    {
        for (auto iter = adjacencies_[at_node].begin();
             iter != adjacencies_[at_node].end();
             ++iter)
        {
            if (*iter == index)
            {
                adjacencies_[at_node].swap_erase(iter);
                break;
            }
        }
    }

    std::vector<Node> nodes_;
    std::vector<AdjacencyArray> adjacencies_;
};

inline int make_id(int node, int attribute) { return (node << 16) | attribute; }

class ColorNodeEditor
{
public:
    ColorNodeEditor() = default;
    ~ColorNodeEditor() = default;

    void show()
    {
        ImGui::Begin("Node language example");
        imnodes::BeginNodeEditor();

        for (const auto& node : output_nodes_)
        {
            const float node_width = 100.0f;
            imnodes::BeginNode(node.out);
            imnodes::Name("output");

            ImGui::Dummy(ImVec2(node_width, 0.f));
            {
                imnodes::BeginAttribute(
                    make_id(int(node.out), 0), imnodes::AttributeType_Input);
                const float label_width = ImGui::CalcTextSize("r").x;
                ImGui::Text("r");
                ImGui::SameLine();
                ImGui::PushItemWidth(node_width - label_width);
                ImGui::DragFloat(
                    "##hidelabel",
                    &node_graph_[node.red].number,
                    0.01f,
                    0.f,
                    1.0f);
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }

            ImGui::Spacing();

            {
                imnodes::BeginAttribute(
                    make_id(int(node.out), 1), imnodes::AttributeType_Input);
                const float label_width = ImGui::CalcTextSize("g").x;
                ImGui::Text("g");
                ImGui::SameLine();
                ImGui::PushItemWidth(node_width - label_width);
                ImGui::DragFloat(
                    "##hidelabel",
                    &node_graph_[node.green].number,
                    0.01f,
                    0.f,
                    1.f);
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }

            ImGui::Spacing();

            {
                imnodes::BeginAttribute(
                    make_id(int(node.out), 2), imnodes::AttributeType_Input);
                const float label_width = ImGui::CalcTextSize("b").x;
                ImGui::Text("b");
                ImGui::SameLine();
                ImGui::PushItemWidth(node_width - label_width);
                ImGui::DragFloat(
                    "##hidelabel",
                    &node_graph_[node.blue].number,
                    0.01f,
                    0.f,
                    1.0f);
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }
            imnodes::EndNode();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
        if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseHoveringWindow() &&
            ImGui::IsMouseClicked(1))
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

                node.red = node_graph_.add_node(num);
                node.green = node_graph_.add_node(num);
                node.blue = node_graph_.add_node(num);
                node.out = node_graph_.add_node(out);

                output_nodes_.push_back(node);

                node_graph_.add_edge(node.red, node.out);
                node_graph_.add_edge(node.green, node.out);
                node_graph_.add_edge(node.blue, node.out);
            }

            if (ImGui::MenuItem("time"))
            {
                // TODO
            }

            if (ImGui::MenuItem("sine"))
            {
                // TODO
            }

            if (ImGui::MenuItem("multiply"))
            {
                // TODO
            }

            if (ImGui::MenuItem("add"))
            {
                // TODO
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();

        imnodes::EndNodeEditor();
        ImGui::End();

        ImU32 color = IM_COL32(255, 20, 147, 255);
        if (output_nodes_.size() > 0u)
        {
            color = node_graph_.evaluate(output_nodes_[0u].out);
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

    Graph node_graph_;
    std::vector<OutputNode> output_nodes_;
}; // namespace

static ColorNodeEditor node_lang;
} // namespace

void NodeEditorInitialize() {}

void NodeEditorShow() { node_lang.show(); }

void NodeEditorShutdown() {}
} // namespace example
