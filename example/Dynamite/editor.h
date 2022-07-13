#pragma once

#include <imgui.h>
#include <imnodes.h>

#include "context.h"

class Editor {

    //ImNodesEditorContext* context = nullptr;
    int current_id = 0;

public:

    Editor() {
        //context = ImNodes::EditorContextCreate();
        //ImNodes::EditorContextSet(context);
    }

    /*~Editor() {
        ImNodes::EditorContextFree(context);
    } */

    static void init() {

        ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);

        ImNodesIO& io = ImNodes::GetIO();
        io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
        io.MultipleSelectModifier.Modifier = &ImGui::GetIO().KeyCtrl;

        ImNodesStyle& style = ImNodes::GetStyle();
        style.Flags |= ImNodesStyleFlags_GridLinesPrimary | ImNodesStyleFlags_GridSnapping;
        //ImFontAtlas::Build
    }

    void show() {

        ImNodes::BeginNodeEditor();

    /* TO DO: integrate context with editor context
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            ImNodes::IsEditorHovered() && ImGui::IsKeyReleased(SDL_SCANCODE_A)) {
            int block_id = ++current_id;
            ImNodes::SetNodeScreenSpacePos(block_id, ImGui::GetMousePos());
            ImNodes::SnapNodeToGrid(block_id);
            printf("block added\n");
            //context.addBlock();
        }
        //context.displayInEditor();
    */
        ImNodes::EndNodeEditor();
    }

    static void exit(ImNodesEditorContext* context) {
        ImNodes::EditorContextFree(context);
        ImNodes::PopAttributeFlag();
    }
};