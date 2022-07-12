// Initial block interaction for Project Dynamite 
// 1. Click on a node and highlight it

#include <imgui.h>
#include <imnodes.h>
#include <imnodes_internal.h>
#include <SDL_scancode.h>
#include "dynamite_editor.h"

#include <algorithm>
#include <vector>

using namespace std;

namespace Dynamite 
{

struct Block
{
    int id;
    const char* name;

    const char* input;
    const char* output;

    Block(const int i, const char* n, const char* in, const char* o) : id(i), name(n), input(in), output(o) {}
};

struct Link
{
    int id;
    int start_attr, end_attr;
};

struct Editor
{
    ImNodesEditorContext* context = nullptr;
    std::vector<Block>    blocks;
    std::vector<Link>     links;
    int                   current_id = 0;
};

Editor editor;

const char* App_GetName()
{
    return "Block Editor";
}

void App_Initialize()
{
    editor.context = ImNodes::EditorContextCreate();
    ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);

    ImNodesIO& io = ImNodes::GetIO();
    io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
    io.MultipleSelectModifier.Modifier = &ImGui::GetIO().KeyCtrl;

    ImNodesStyle& style = ImNodes::GetStyle();
    style.Flags |= ImNodesStyleFlags_GridLinesPrimary | ImNodesStyleFlags_GridSnapping;
}

void App_Finalize()
{
    ImNodes::PopAttributeFlag();
    ImNodes::EditorContextFree(editor.context);
}

void App_Frame()
{
    //ImGui::ShowDemoWindow();
    ImNodes::EditorContextSet(editor.context);
    
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    static bool use_work_area = true;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
    ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);

    ImGui::Begin("Block Viewer");
    ImGui::TextUnformatted("A -- add node");

    ImNodes::BeginNodeEditor();

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        ImNodes::IsEditorHovered() && ImGui::IsKeyReleased(SDL_SCANCODE_A)) {
        const int block_id = ++editor.current_id;
        ImNodes::SetNodeScreenSpacePos(block_id, ImGui::GetMousePos());
        ImNodes::SnapNodeToGrid(block_id);
        editor.blocks.push_back(Block(block_id, "name", "i", "o")); // load names from block library
    }

    for (Block& block : editor.blocks) {
        ImNodes::BeginNode(block.id);
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted(block.name);
        ImNodes::EndNodeTitleBar();

        ImNodes::BeginInputAttribute(block.id << 8);
        ImGui::TextUnformatted(block.input);
        ImNodes::EndInputAttribute();

        ImNodes::BeginOutputAttribute(block.id << 24);
        const float text_width = ImGui::CalcTextSize(block.output).x;
        ImGui::Indent(120.f + ImGui::CalcTextSize("value").x - text_width);
        ImGui::TextUnformatted(block.output);
        ImNodes::EndOutputAttribute();  

        ImNodes::EndNode();
    }   

    for (const Link& link : editor.links) {
        ImNodes::Link(link.id, link.start_attr, link.end_attr);
    }

    // edit menu popup only visible for node last added to the canvas
    if (ImGui::BeginPopupContextItem("my popup")) {
        if (ImGui::MenuItem("Bypass")) {
            printf("Bypass\n");
        }
        if (ImGui::MenuItem("Other option")) {
            printf("Other option\n");
        }
        ImGui::EndPopup();
    } 

    /* ImGui::OpenPopup("my popup", ImGuiPopupFlags_MouseButtonRight);
    if (ImGui::BeginPopup("my popup")) {
        if (ImGui::MenuItem("Bypass")) {
            printf("Bypass\n");
        }
        if (ImGui::MenuItem("Other option")) {
            printf("Other option\n");
        }
        ImGui::EndMenu();

        ImGui::EndPopup();
    } */

    ImNodes::EndNodeEditor();

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


}