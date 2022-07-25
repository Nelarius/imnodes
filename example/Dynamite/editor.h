#pragma once

#include <imgui.h>
#include <imnodes.h>
#include <stdio.h>

#include "context.h"

class Editor {

public:

    static void Editor::init() {
        ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);

        ImNodesIO& io = ImNodes::GetIO();
        io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
        io.MultipleSelectModifier.Modifier = &ImGui::GetIO().KeyCtrl;

        ImNodesStyle& style = ImNodes::GetStyle();
        style.Flags |= ImNodesStyleFlags_GridLinesPrimary | ImNodesStyleFlags_GridSnapping;
        //ImFontAtlas::Build
    }

    static void Editor::exit() {
        ImNodes::PopAttributeFlag();
    }

    Editor();
    void show(Context &m_context);
    void displayInEditor(Context m_context);
    void clickHandler();
    void showPopup();
    int isBlockHovered();
};