#include <imgui.h>
#include <imnodes.h>
#include "block.h"

#include <SDL_scancode.h>

struct Link {
    int id;
    int start_attr, end_attr;
};

class Context {
    public: 

    ImNodesEditorContext* context = nullptr;
    std::vector<Block>    blocks;
    std::vector<Link>     links;
    int                   current_id = 0;

    /*for (const Link& link : editor.links) {
        ImNodes::Link(link.id, link.start_attr, link.end_attr);
    } */

    Context(ImNodesEditorContext* ctx) : context(ctx) { 
        context = ImNodes::EditorContextCreate();
        ImNodes::EditorContextSet(context); 
    }

    void update() {
        // if block is added to the canvas, for now by pressing A
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        ImNodes::IsEditorHovered() && ImGui::IsKeyReleased(SDL_SCANCODE_A)) {
            const int block_id = ++current_id;

            /* ImNodes::SetNodeScreenSpacePos(block_id, ImGui::GetMousePos());
            ImNodes::SnapNodeToGrid(block_id); */ //add to canvas

            blocks.push_back(Block(block_id, "DSPBlock", "IN", "OUT")); // load names from block library
        }
    }


};