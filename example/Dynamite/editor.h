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

    void show(Context &m_context) {
        ImNodes::BeginNodeEditor();
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            ImNodes::IsEditorHovered() && ImGui::IsKeyReleased(SDL_SCANCODE_A)) {
            m_context.update();
        } // here is where the m_context is resetting itself
        displayInEditor(m_context);
        ImNodes::EndNodeEditor();
    }

    void displayInEditor(Context m_context) {
        for (Block& block : m_context._blocks) {
            block.show();
        } 

        for (const Link& link : m_context._links) {
            ImNodes::Link(link.id, link.start_attr, link.end_attr);
        }

    }

    static void exit() {
        ImNodes::PopAttributeFlag();
    }
};