#pragma once

#include <imgui.h>
#include <imnodes.h>
#include <stdio.h>

#include "context.h"
#include "graph.h"

class Editor {

public:
    static void init() {
        ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);

        ImNodesIO& io = ImNodes::GetIO();
        io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
        io.MultipleSelectModifier.Modifier = &ImGui::GetIO().KeyCtrl;

        ImNodesStyle& style = ImNodes::GetStyle();
        style.Flags |= ImNodesStyleFlags_GridLinesPrimary | ImNodesStyleFlags_GridSnapping;
    }

    static void exit() {
        ImNodes::PopAttributeFlag();
    }

    Editor();
    void show(Context &m_context);
    void displayInEditor(Graph graph);
    void clickHandler();
    void showPopup(Graph &graph);
    void deletePort(Graph &graph);
    int isBlockClicked();
};