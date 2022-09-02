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

    Editor() = default;

    // Displays the editor, called every frame
    // Waits for keystroke and mouse events to render and remove items from the central editor
    void show(Context &m_context);

    // Iterator for _blocks and _links to display in the editor
    void displayInEditor(Graph graph);

    void clickHandler();

    // Describes the edit menu for individual blocks in the editor
    void showPopup(Graph &graph);

    // TO DO : need to move ImNodes conditional statement from Block::deleteInport/deleteOutport to Editor::show
    // Call every frame to check if any ports are deleted
    void deletePort(Graph &graph);

    // Helper for determining if and which block id was deleted from the editor
    int isBlockClicked();
};