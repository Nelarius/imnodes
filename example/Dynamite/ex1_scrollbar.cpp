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
    int   id;
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
    ImNodesEditorContext* context = nullptr;
    std::vector<Node>     nodes;
    std::vector<Link>     links;
    int                   current_id = 0;
};

void ClearEditor(Editor editor);
void HoverTools(Editor editor);

void show_editor(const char* editor_name, Editor& editor)
{
    ImNodes::EditorContextSet(editor.context);

    ImGui::Begin(editor_name);
    ImGui::TextUnformatted("A -- add node");

    ImGui::SameLine();
    static int clicked = 0;
    if (ImGui::Button("clear"))
        clicked++;
    if (clicked & 1)
    { 
        for (size_t i=0; i < editor.nodes.size(); i++)
            editor.nodes.erase(editor.nodes.begin()+i); 
    }

    // window layout
    static bool disable_mouse_wheel = false;

    static int block_clicked = 0;
     // Child 1: no border, enable horizontal scrollbar
    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        if (disable_mouse_wheel)
            window_flags |= ImGuiWindowFlags_NoScrollWithMouse;
        ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x * 0.2f, 100), false, window_flags);
        for (int i = 0; i < 10; i++)
        {
            //ImGui::Text("block #%d", i);
            ImGui::Button("block");
            if (ImGui::Button("block"))
                block_clicked++;
        }
        ImGui::EndChild();
    }

    ImNodes::BeginNodeEditor();

    if ((ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        ImNodes::IsEditorHovered() && ImGui::IsKeyReleased(SDL_SCANCODE_A)) || (block_clicked & 1))
    {
        block_clicked = 0;
        const int node_id = ++editor.current_id;
        ImNodes::SetNodeScreenSpacePos(node_id, ImGui::GetMousePos());
        ImNodes::SnapNodeToGrid(node_id);
        editor.nodes.push_back(Node(node_id, 0.f));
    }

    for (Node& node : editor.nodes)
    {
        //printf("%d\n", node.id);

        ImNodes::BeginNode(node.id);

        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("DSP Block");
        ImNodes::EndNodeTitleBar();

        ImNodes::BeginInputAttribute(node.id << 8);
        ImGui::TextUnformatted("INPUT_L");
        ImNodes::EndInputAttribute();

        ImNodes::BeginInputAttribute(node.id << 8);
        ImGui::TextUnformatted("INPUT_R");
        ImNodes::EndInputAttribute();

        ImNodes::BeginStaticAttribute(node.id << 16);
        ImGui::PushItemWidth(120.0f);
        ImGui::DragFloat("delay", &node.value, 0.01f);
        ImGui::PopItemWidth();
        ImNodes::EndStaticAttribute();

        ImNodes::BeginOutputAttribute(node.id << 24);
        const float text_width_l = ImGui::CalcTextSize("OUTPUT_L").x;
        ImGui::Indent(120.f + ImGui::CalcTextSize("value").x - text_width_l);
        ImGui::TextUnformatted("OUTPUT_L");
        ImNodes::EndOutputAttribute();

        ImNodes::BeginOutputAttribute(node.id << 24);
        const float text_width_r = ImGui::CalcTextSize("OUTPUT_R").x;
        ImGui::Indent(120.f + ImGui::CalcTextSize("value").x - text_width_r);
        ImGui::TextUnformatted("OUTPUT_R");
        ImNodes::EndOutputAttribute();

        ImNodes::EndNode(); 
    } 

    for (const Link& link : editor.links)
    {
        ImNodes::Link(link.id, link.start_attr, link.end_attr);
    }

    ImNodes::EndNodeEditor();

    int nodeid = 0;
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImNodes::IsNodeHovered(&nodeid))
        HoverTools(editor);

    {
        Link link;
        if (ImNodes::IsLinkCreated(&link.start_attr, &link.end_attr))
        {
            link.id = ++editor.current_id;
            editor.links.push_back(link);
        }
    }

    {
        int link_id;
        if (ImNodes::IsLinkDestroyed(&link_id))
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
//Editor editor2;

// clears all nodes on the editor canvas
void ClearEditor(Editor editor)
{
    printf("inside clear editor function\n");
    for (size_t i=0; i < editor.nodes.size(); i++)
    {
        printf("node id = %zu\n", i);
        editor.nodes.erase(editor.nodes.begin()+i); 
    }
}

void HoverTools(Editor editor)
{
   /* for (size_t i=0; i < editor.nodes.size(); i++)
        {
            if(nodeid == editor.nodes[i].id)
            { */
                static int selected_fish = -1;
                const char* names[] = { "add field", "delete block"};
                static bool toggles[] = {true, false};
                ImGui::OpenPopup("Tools");
                if (ImGui::BeginPopup("Tools"))
                {
                    for (int k = 0; k < IM_ARRAYSIZE(names); k++)
                        ImGui::MenuItem(names[k], "", &toggles[k]);
                    if (ImGui::BeginMenu("Sub-menu"))
                    {
                        ImGui::MenuItem("Click me");
                        ImGui::EndMenu();
                    }

                    ImGui::Separator();
                    ImGui::Text("Tooltip here");
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("I am a tooltip over a popup");

                    if (ImGui::Button("Stacked Popup"))
                        ImGui::OpenPopup("another popup");
                    if (ImGui::BeginPopup("another popup"))
                    {
                        for (int k = 0; k < IM_ARRAYSIZE(names); k++)
                            ImGui::MenuItem(names[k], "", &toggles[k]);
                        if (ImGui::BeginMenu("Sub-menu"))
                        {
                            ImGui::MenuItem("Click me");
                            if (ImGui::Button("Stacked Popup"))
                                ImGui::OpenPopup("another popup");
                            if (ImGui::BeginPopup("another popup"))
                            {
                                ImGui::Text("I am the last one here.");
                                ImGui::EndPopup();
                            }
                            ImGui::EndMenu();
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::EndPopup();
                }
            //}
        //}
}

 /*
        // hover tool
        static int selected_parameter = -1;
        const char* names[] = { "delay", "gain" };

        if (ImNodes::IsNodeHovered(&node.id))
            ImGui::OpenPopup("Add parameters");
            printf("popup enabled\n");
        ImGui::SameLine();
        ImGui::TextUnformatted(selected_parameter == -1 ? "<None>" : names[selected_parameter]);
        if (ImGui::BeginPopup("Add parameters"))
        {
            ImGui::Separator();
            for (int i = 0; i < IM_ARRAYSIZE(names); i++)
                if (ImGui::Selectable(names[i]))
                    selected_parameter = i;
            ImGui::EndPopup();

            ImNodes::BeginStaticAttribute(node.id << 16);
            ImGui::PushItemWidth(120.0f);
            ImGui::DragFloat("another parameter", &node.value, 0.01f);
            ImGui::PopItemWidth();
            ImNodes::EndStaticAttribute(); 
        } 
        */

} // namespace

void NodeEditorInitialize()
{
    editor1.context = ImNodes::EditorContextCreate();
    //editor2.context = ImNodes::EditorContextCreate();
    ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);

    ImNodesIO& io = ImNodes::GetIO();
    io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
    io.MultipleSelectModifier.Modifier = &ImGui::GetIO().KeyCtrl;

    ImNodesStyle& style = ImNodes::GetStyle();
    style.Flags |= ImNodesStyleFlags_GridLinesPrimary | ImNodesStyleFlags_GridSnapping;
}

void NodeEditorShow()
{
    show_editor("editor1", editor1);
    //show_editor("editor2", editor2);
}

void NodeEditorShutdown()
{
    ImNodes::PopAttributeFlag();
    ImNodes::EditorContextFree(editor1.context);
    //ImNodes::EditorContextFree(editor2.context);
}

} //namespace example
