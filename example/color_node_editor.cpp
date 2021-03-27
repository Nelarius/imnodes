#include "node_editor.h"
#include "graph.h"

#include <imnodes.h>
#include <imgui.h>

#include <SDL_keycode.h>
#include <SDL_timer.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <vector>

namespace example
{
namespace
{
enum class NodeType
{
    add,
    multiply,
    output,
    sine,
    time,
    value
};

struct Node
{
    NodeType type;
    float value;

    explicit Node(const NodeType t) : type(t), value(0.f) {}

    Node(const NodeType t, const float v) : type(t), value(v) {}
};

template<class T>
T clamp(T x, T a, T b)
{
    return std::min(b, std::max(x, a));
}

static float current_time_seconds = 0.f;
static bool emulate_three_button_mouse = false;

ImU32 evaluate(const Graph<Node>& graph, const int root_node)
{
    std::stack<int> postorder;
    dfs_traverse(
        graph, root_node, [&postorder](const int node_id) -> void { postorder.push(node_id); });

    std::stack<float> value_stack;
    while (!postorder.empty())
    {
        const int id = postorder.top();
        postorder.pop();
        const Node node = graph.node(id);

        switch (node.type)
        {
        case NodeType::add:
        {
            const float rhs = value_stack.top();
            value_stack.pop();
            const float lhs = value_stack.top();
            value_stack.pop();
            value_stack.push(lhs + rhs);
        }
        break;
        case NodeType::multiply:
        {
            const float rhs = value_stack.top();
            value_stack.pop();
            const float lhs = value_stack.top();
            value_stack.pop();
            value_stack.push(rhs * lhs);
        }
        break;
        case NodeType::sine:
        {
            const float x = value_stack.top();
            value_stack.pop();
            const float res = std::abs(std::sin(x));
            value_stack.push(res);
        }
        break;
        case NodeType::time:
        {
            value_stack.push(current_time_seconds);
        }
        break;
        case NodeType::value:
        {
            // If the edge does not have an edge connecting to another node, then just use the value
            // at this node. It means the node's input pin has not been connected to anything and
            // the value comes from the node's UI.
            if (graph.num_edges_from_node(id) == 0ull)
            {
                value_stack.push(node.value);
            }
        }
        break;
        default:
            break;
        }
    }

    // The final output node isn't evaluated in the loop -- instead we just pop
    // the three values which should be in the stack.
    assert(value_stack.size() == 3ull);
    const int b = static_cast<int>(255.f * clamp(value_stack.top(), 0.f, 1.f) + 0.5f);
    value_stack.pop();
    const int g = static_cast<int>(255.f * clamp(value_stack.top(), 0.f, 1.f) + 0.5f);
    value_stack.pop();
    const int r = static_cast<int>(255.f * clamp(value_stack.top(), 0.f, 1.f) + 0.5f);
    value_stack.pop();

    return IM_COL32(r, g, b, 255);
}

class ColorNodeEditor
{
public:
    ColorNodeEditor() : graph_(), nodes_(), root_node_id_(-1) {}

    void show()
    {
        // Update timer context
        current_time_seconds = 0.001f * SDL_GetTicks();

        // The node editor window
        ImGui::Begin("color node editor");
        ImGui::TextUnformatted("Edit the color of the output color window using nodes.");
        ImGui::Columns(2);
        ImGui::TextUnformatted("A -- add node");
        ImGui::TextUnformatted("X -- delete selected node or link");
        ImGui::NextColumn();
        if (ImGui::Checkbox("emulate_three_button_mouse", &emulate_three_button_mouse))
        {
            imnodes::GetIO().emulate_three_button_mouse.modifier =
                emulate_three_button_mouse ? &ImGui::GetIO().KeyAlt : NULL;
        }
        ImGui::Columns(1);

        imnodes::BeginNodeEditor();

        // Handle new nodes
        // These are driven by the user, so we place this code before rendering the nodes
        {
            const bool open_popup = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
                                    imnodes::IsEditorHovered() &&
                                    ImGui::IsKeyReleased(SDL_SCANCODE_A);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
            if (!ImGui::IsAnyItemHovered() && open_popup)
            {
                ImGui::OpenPopup("add node");
            }

            if (ImGui::BeginPopup("add node"))
            {
                const ImVec2 click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();

                if (ImGui::MenuItem("add"))
                {
                    const Node value(NodeType::value, 0.f);
                    const Node op(NodeType::add);

                    UiNode ui_node;
                    ui_node.type = UiNodeType::add;
                    ui_node.add.lhs = graph_.insert_node(value);
                    ui_node.add.rhs = graph_.insert_node(value);
                    ui_node.id = graph_.insert_node(op);

                    graph_.insert_edge(ui_node.id, ui_node.add.lhs);
                    graph_.insert_edge(ui_node.id, ui_node.add.rhs);

                    nodes_.push_back(ui_node);
                    imnodes::SetNodeScreenSpacePos(ui_node.id, click_pos);
                }

                if (ImGui::MenuItem("multiply"))
                {
                    const Node value(NodeType::value, 0.f);
                    const Node op(NodeType::multiply);

                    UiNode ui_node;
                    ui_node.type = UiNodeType::multiply;
                    ui_node.multiply.lhs = graph_.insert_node(value);
                    ui_node.multiply.rhs = graph_.insert_node(value);
                    ui_node.id = graph_.insert_node(op);

                    graph_.insert_edge(ui_node.id, ui_node.multiply.lhs);
                    graph_.insert_edge(ui_node.id, ui_node.multiply.rhs);

                    nodes_.push_back(ui_node);
                    imnodes::SetNodeScreenSpacePos(ui_node.id, click_pos);
                }

                if (ImGui::MenuItem("output") && root_node_id_ == -1)
                {
                    const Node value(NodeType::value, 0.f);
                    const Node out(NodeType::output);

                    UiNode ui_node;
                    ui_node.type = UiNodeType::output;
                    ui_node.output.r = graph_.insert_node(value);
                    ui_node.output.g = graph_.insert_node(value);
                    ui_node.output.b = graph_.insert_node(value);
                    ui_node.id = graph_.insert_node(out);

                    graph_.insert_edge(ui_node.id, ui_node.output.r);
                    graph_.insert_edge(ui_node.id, ui_node.output.g);
                    graph_.insert_edge(ui_node.id, ui_node.output.b);

                    nodes_.push_back(ui_node);
                    imnodes::SetNodeScreenSpacePos(ui_node.id, click_pos);
                    root_node_id_ = ui_node.id;
                }

                if (ImGui::MenuItem("sine"))
                {
                    const Node value(NodeType::value, 0.f);
                    const Node op(NodeType::sine);

                    UiNode ui_node;
                    ui_node.type = UiNodeType::sine;
                    ui_node.sine.input = graph_.insert_node(value);
                    ui_node.id = graph_.insert_node(op);

                    graph_.insert_edge(ui_node.id, ui_node.sine.input);

                    nodes_.push_back(ui_node);
                    imnodes::SetNodeScreenSpacePos(ui_node.id, click_pos);
                }

                if (ImGui::MenuItem("time"))
                {
                    UiNode ui_node;
                    ui_node.type = UiNodeType::time;
                    ui_node.id = graph_.insert_node(Node(NodeType::time));

                    nodes_.push_back(ui_node);
                    imnodes::SetNodeScreenSpacePos(ui_node.id, click_pos);
                }

                ImGui::EndPopup();
            }
            ImGui::PopStyleVar();
        }

        for (const UiNode& node : nodes_)
        {
            switch (node.type)
            {
            case UiNodeType::add:
            {
                const float node_width = 100.f;
                imnodes::BeginNode(node.id);

                imnodes::BeginNodeTitleBar();
                ImGui::TextUnformatted("add");
                imnodes::EndNodeTitleBar();
                {
                    imnodes::BeginInputAttribute(node.add.lhs);
                    const float label_width = ImGui::CalcTextSize("left").x;
                    ImGui::TextUnformatted("left");
                    if (graph_.num_edges_from_node(node.add.lhs) == 0ull)
                    {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(node_width - label_width);
                        ImGui::DragFloat("##hidelabel", &graph_.node(node.add.lhs).value, 0.01f);
                        ImGui::PopItemWidth();
                    }
                    imnodes::EndInputAttribute();
                }

                {
                    imnodes::BeginInputAttribute(node.add.rhs);
                    const float label_width = ImGui::CalcTextSize("right").x;
                    ImGui::TextUnformatted("right");
                    if (graph_.num_edges_from_node(node.add.rhs) == 0ull)
                    {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(node_width - label_width);
                        ImGui::DragFloat("##hidelabel", &graph_.node(node.add.rhs).value, 0.01f);
                        ImGui::PopItemWidth();
                    }
                    imnodes::EndInputAttribute();
                }

                ImGui::Spacing();

                {
                    imnodes::BeginOutputAttribute(node.id);
                    const float label_width = ImGui::CalcTextSize("result").x;
                    ImGui::Indent(node_width - label_width);
                    ImGui::TextUnformatted("result");
                    imnodes::EndOutputAttribute();
                }

                imnodes::EndNode();
            }
            break;
            case UiNodeType::multiply:
            {
                const float node_width = 100.0f;
                imnodes::BeginNode(node.id);

                imnodes::BeginNodeTitleBar();
                ImGui::TextUnformatted("multiply");
                imnodes::EndNodeTitleBar();

                {
                    imnodes::BeginInputAttribute(node.multiply.lhs);
                    const float label_width = ImGui::CalcTextSize("left").x;
                    ImGui::TextUnformatted("left");
                    if (graph_.num_edges_from_node(node.multiply.lhs) == 0ull)
                    {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(node_width - label_width);
                        ImGui::DragFloat(
                            "##hidelabel", &graph_.node(node.multiply.lhs).value, 0.01f);
                        ImGui::PopItemWidth();
                    }
                    imnodes::EndInputAttribute();
                }

                {
                    imnodes::BeginInputAttribute(node.multiply.rhs);
                    const float label_width = ImGui::CalcTextSize("right").x;
                    ImGui::TextUnformatted("right");
                    if (graph_.num_edges_from_node(node.multiply.rhs) == 0ull)
                    {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(node_width - label_width);
                        ImGui::DragFloat(
                            "##hidelabel", &graph_.node(node.multiply.rhs).value, 0.01f);
                        ImGui::PopItemWidth();
                    }
                    imnodes::EndInputAttribute();
                }

                ImGui::Spacing();

                {
                    imnodes::BeginOutputAttribute(node.id);
                    const float label_width = ImGui::CalcTextSize("result").x;
                    ImGui::Indent(node_width - label_width);
                    ImGui::TextUnformatted("result");
                    imnodes::EndOutputAttribute();
                }

                imnodes::EndNode();
            }
            break;
            case UiNodeType::output:
            {
                const float node_width = 100.0f;
                imnodes::PushColorStyle(imnodes::ColorStyle_TitleBar, IM_COL32(11, 109, 191, 255));
                imnodes::PushColorStyle(
                    imnodes::ColorStyle_TitleBarHovered, IM_COL32(45, 126, 194, 255));
                imnodes::PushColorStyle(
                    imnodes::ColorStyle_TitleBarSelected, IM_COL32(81, 148, 204, 255));
                imnodes::BeginNode(node.id);

                imnodes::BeginNodeTitleBar();
                ImGui::TextUnformatted("output");
                imnodes::EndNodeTitleBar();

                ImGui::Dummy(ImVec2(node_width, 0.f));
                {
                    imnodes::BeginInputAttribute(node.output.r);
                    const float label_width = ImGui::CalcTextSize("r").x;
                    ImGui::TextUnformatted("r");
                    if (graph_.num_edges_from_node(node.output.r) == 0ull)
                    {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(node_width - label_width);
                        ImGui::DragFloat(
                            "##hidelabel", &graph_.node(node.output.r).value, 0.01f, 0.f, 1.0f);
                        ImGui::PopItemWidth();
                    }
                    imnodes::EndInputAttribute();
                }

                ImGui::Spacing();

                {
                    imnodes::BeginInputAttribute(node.output.g);
                    const float label_width = ImGui::CalcTextSize("g").x;
                    ImGui::TextUnformatted("g");
                    if (graph_.num_edges_from_node(node.output.g) == 0ull)
                    {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(node_width - label_width);
                        ImGui::DragFloat(
                            "##hidelabel", &graph_.node(node.output.g).value, 0.01f, 0.f, 1.f);
                        ImGui::PopItemWidth();
                    }
                    imnodes::EndInputAttribute();
                }

                ImGui::Spacing();

                {
                    imnodes::BeginInputAttribute(node.output.b);
                    const float label_width = ImGui::CalcTextSize("b").x;
                    ImGui::TextUnformatted("b");
                    if (graph_.num_edges_from_node(node.output.b) == 0ull)
                    {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(node_width - label_width);
                        ImGui::DragFloat(
                            "##hidelabel", &graph_.node(node.output.b).value, 0.01f, 0.f, 1.0f);
                        ImGui::PopItemWidth();
                    }
                    imnodes::EndInputAttribute();
                }
                imnodes::EndNode();
                imnodes::PopColorStyle();
                imnodes::PopColorStyle();
                imnodes::PopColorStyle();
            }
            break;
            case UiNodeType::sine:
            {
                const float node_width = 100.0f;
                imnodes::BeginNode(node.id);

                imnodes::BeginNodeTitleBar();
                ImGui::TextUnformatted("sine");
                imnodes::EndNodeTitleBar();

                {
                    imnodes::BeginInputAttribute(node.sine.input);
                    const float label_width = ImGui::CalcTextSize("number").x;
                    ImGui::TextUnformatted("number");
                    if (graph_.num_edges_from_node(node.sine.input) == 0ull)
                    {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(node_width - label_width);
                        ImGui::DragFloat(
                            "##hidelabel", &graph_.node(node.sine.input).value, 0.01f, 0.f, 1.0f);
                        ImGui::PopItemWidth();
                    }
                    imnodes::EndInputAttribute();
                }

                ImGui::Spacing();

                {
                    imnodes::BeginOutputAttribute(node.id);
                    const float label_width = ImGui::CalcTextSize("output").x;
                    ImGui::Indent(node_width - label_width);
                    ImGui::TextUnformatted("output");
                    imnodes::EndInputAttribute();
                }

                imnodes::EndNode();
            }
            break;
            case UiNodeType::time:
            {
                imnodes::BeginNode(node.id);

                imnodes::BeginNodeTitleBar();
                ImGui::TextUnformatted("time");
                imnodes::EndNodeTitleBar();

                imnodes::BeginOutputAttribute(node.id);
                ImGui::Text("output");
                imnodes::EndOutputAttribute();

                imnodes::EndNode();
            }
            break;
            }
        }

        for (const auto& edge : graph_.edges())
        {
            // If edge doesn't start at value, then it's an internal edge, i.e.
            // an edge which links a node's operation to its input. We don't
            // want to render node internals with visible links.
            if (graph_.node(edge.from).type != NodeType::value)
                continue;

            imnodes::Link(edge.id, edge.from, edge.to);
        }

        imnodes::EndNodeEditor();

        // Handle new links
        // These are driven by Imnodes, so we place the code after EndNodeEditor().

        {
            int start_attr, end_attr;
            if (imnodes::IsLinkCreated(&start_attr, &end_attr))
            {
                const NodeType start_type = graph_.node(start_attr).type;
                const NodeType end_type = graph_.node(end_attr).type;

                const bool valid_link = start_type != end_type;
                if (valid_link)
                {
                    // Ensure the edge is always directed from the value to
                    // whatever produces the value
                    if (start_type != NodeType::value)
                    {
                        std::swap(start_attr, end_attr);
                    }
                    graph_.insert_edge(start_attr, end_attr);
                }
            }
        }

        // Handle deleted links

        {
            int link_id;
            if (imnodes::IsLinkDestroyed(&link_id))
            {
                graph_.erase_edge(link_id);
            }
        }

        {
            const int num_selected = imnodes::NumSelectedLinks();
            if (num_selected > 0 && ImGui::IsKeyReleased(SDL_SCANCODE_X))
            {
                static std::vector<int> selected_links;
                selected_links.resize(static_cast<size_t>(num_selected));
                imnodes::GetSelectedLinks(selected_links.data());
                for (const int edge_id : selected_links)
                {
                    graph_.erase_edge(edge_id);
                }
            }
        }

        {
            const int num_selected = imnodes::NumSelectedNodes();
            if (num_selected > 0 && ImGui::IsKeyReleased(SDL_SCANCODE_X))
            {
                static std::vector<int> selected_nodes;
                selected_nodes.resize(static_cast<size_t>(num_selected));
                imnodes::GetSelectedNodes(selected_nodes.data());
                for (const int node_id : selected_nodes)
                {
                    graph_.erase_node(node_id);
                    auto iter = std::find_if(
                        nodes_.begin(), nodes_.end(), [node_id](const UiNode& node) -> bool {
                            return node.id == node_id;
                        });
                    // Erase any additional internal nodes
                    switch (iter->type)
                    {
                    case UiNodeType::add:
                        graph_.erase_node(iter->add.lhs);
                        graph_.erase_node(iter->add.rhs);
                        break;
                    case UiNodeType::multiply:
                        graph_.erase_node(iter->multiply.lhs);
                        graph_.erase_node(iter->multiply.rhs);
                        break;
                    case UiNodeType::output:
                        graph_.erase_node(iter->output.r);
                        graph_.erase_node(iter->output.g);
                        graph_.erase_node(iter->output.b);
                        root_node_id_ = -1;
                        break;
                    case UiNodeType::sine:
                        graph_.erase_node(iter->sine.input);
                        break;
                    default:
                        break;
                    }
                    nodes_.erase(iter);
                }
            }
        }

        ImGui::End();

        // The color output window

        const ImU32 color =
            root_node_id_ != -1 ? evaluate(graph_, root_node_id_) : IM_COL32(255, 20, 147, 255);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, color);
        ImGui::Begin("output color");
        ImGui::End();
        ImGui::PopStyleColor();
    }

private:
    enum class UiNodeType
    {
        add,
        multiply,
        output,
        sine,
        time,
    };

    struct UiNode
    {
        UiNodeType type;
        // The identifying id of the ui node. For add, multiply, sine, and time
        // this is the "operation" node id. The additional input nodes are
        // stored in the structs.
        int id;

        union
        {
            struct
            {
                int lhs, rhs;
            } add;

            struct
            {
                int lhs, rhs;
            } multiply;

            struct
            {
                int r, g, b;
            } output;

            struct
            {
                int input;
            } sine;
        };
    };

    Graph<Node> graph_;
    std::vector<UiNode> nodes_;
    int root_node_id_;
};

static ColorNodeEditor color_editor;
} // namespace

void NodeEditorInitialize()
{
    imnodes::IO& io = imnodes::GetIO();
    io.link_detach_with_modifier_click.modifier = &ImGui::GetIO().KeyCtrl;
}

void NodeEditorShow() { color_editor.show(); }

void NodeEditorShutdown() {}
} // namespace example
