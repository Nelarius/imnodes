#pragma once

#include <imgui.h>
#include <imnodes.h>

#include "context.h"

class Editor {

public:

    Editor() { }

    static void init() {

        ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);

        ImNodesIO& io = ImNodes::GetIO();
        io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
        io.MultipleSelectModifier.Modifier = &ImGui::GetIO().KeyCtrl;

        ImNodesStyle& style = ImNodes::GetStyle();
        style.Flags |= ImNodesStyleFlags_GridLinesPrimary | ImNodesStyleFlags_GridSnapping;
        //ImFontAtlas::Build
    }

    void show(Context &m_context, float s_RightPaneSize) {
        ImGui::BeginChild("##central canvas", ImVec2(s_RightPaneSize, -1), false, 0); 
        ImNodes::BeginNodeEditor();
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            ImNodes::IsEditorHovered() && ImGui::IsKeyReleased(SDL_SCANCODE_A)) {
            m_context.update();
        }
        displayInEditor(m_context);
        ImNodes::EndNodeEditor();

        int block_id = 0;
        if (ImNodes::IsNodeSelected(block_id)) { 
            printf("block has been selected. Diplay info in the multipurpose panel\n");
        }

        ImGui::EndChild();
    }

    void showPopup() {
         // displays as many popups as blocks on the board, might be a weird ImGui bug
        if (ImGui::BeginPopupContextItem("my popup")) {
            if (ImGui::MenuItem("Bypass")) {
                printf("Bypass\n");
            }
            if (ImGui::MenuItem("Other option")) {
                printf("Other option\n");
            }
            ImGui::EndPopup();
        } 
    }

    void displayInEditor(Context m_context) {
        for (Block& block : m_context._blocks) {
            block.show();
            showPopup();
            
            // id node is clicked, 
            // MultiPanel::showInfo
            //ImGui::IsItemClicked();
        }  

        for (const Link& link : m_context._links) {
            ImNodes::Link(link.id, link.start_attr, link.end_attr);
        }

    }

    static void exit() {
        ImNodes::PopAttributeFlag();
    }
};