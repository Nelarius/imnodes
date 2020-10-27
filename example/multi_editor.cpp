#include "node_editor.h"
#include <imnodes.h>
#include <imgui.h>
#include <SDL_scancode.h>

#include <algorithm>
#include <vector>

namespace example
{
namespace
{
struct Node
{
    int id;
    float value;

    Node(const int i, const float v) : id(i), value(v) {}
};

struct Link
{
    int id;
    int start_attr, end_attr;
};

struct Editor
{
    imnodes::EditorContext* context = nullptr;
    std::vector<Node> nodes;
    std::vector<Link> links;
    int current_id = 0;
};

void show_editor(const char* editor_name, Editor& editor)
{
    imnodes::EditorContextSet(editor.context);

    ImGui::Begin(editor_name);
    ImGui::TextUnformatted("A -- add node");

    imnodes::BeginNodeEditor();

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        imnodes::IsEditorHovered() && ImGui::IsKeyReleased(SDL_SCANCODE_A))
    {
        const int node_id = ++editor.current_id;
        imnodes::SetNodeScreenSpacePos(node_id, ImGui::GetMousePos());
        editor.nodes.push_back(Node(node_id, 0.f));
    }

    for (Node& node : editor.nodes)
    {
        imnodes::BeginNode(node.id);

        imnodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("node");
        imnodes::EndNodeTitleBar();

        imnodes::BeginInputAttribute(node.id << 8);
        ImGui::TextUnformatted("input");
        imnodes::EndInputAttribute();

        imnodes::BeginStaticAttribute(node.id << 16);
        ImGui::PushItemWidth(120.0f);
        ImGui::DragFloat("value", &node.value, 0.01f);
        ImGui::PopItemWidth();
        imnodes::EndStaticAttribute();

        imnodes::BeginOutputAttribute(node.id << 24);
        const float text_width = ImGui::CalcTextSize("output").x;
        ImGui::Indent(120.f + ImGui::CalcTextSize("value").x - text_width);
        ImGui::TextUnformatted("output");
        imnodes::EndOutputAttribute();

        imnodes::EndNode();
    }

    for (const Link& link : editor.links)
    {
        imnodes::Link(link.id, link.start_attr, link.end_attr);
    }

    imnodes::EndNodeEditor();

    {
        Link link;
        if (imnodes::IsLinkCreated(&link.start_attr, &link.end_attr))
        {
            link.id = ++editor.current_id;
            editor.links.push_back(link);
        }
    }

    {
        int link_id;
        if (imnodes::IsLinkDestroyed(&link_id))
        {
            auto iter = std::find_if(
                editor.links.begin(), editor.links.end(), [link_id](const Link& link) -> bool {
                    return link.id == link_id;
                });
            assert(iter != editor.links.end());
            editor.links.erase(iter);
        }
    }

    ImGui::End();
}

Editor editor1;
Editor editor2;
} // namespace

void NodeEditorInitialize()
{
    editor1.context = imnodes::EditorContextCreate();
    editor2.context = imnodes::EditorContextCreate();
    imnodes::PushAttributeFlag(imnodes::AttributeFlags_EnableLinkDetachWithDragClick);

    imnodes::IO& io = imnodes::GetIO();
    io.link_detach_with_modifier_click.modifier = &ImGui::GetIO().KeyCtrl;
}

void NodeEditorShow()
{
    show_editor("editor1", editor1);
    show_editor("editor2", editor2);
}

void NodeEditorShutdown()
{
    imnodes::PopAttributeFlag();
    imnodes::EditorContextFree(editor1.context);
    imnodes::EditorContextFree(editor2.context);
}
} // namespace example
