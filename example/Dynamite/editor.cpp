#include "editor.h"
#include <string>

#include "palette.h"
// Retrieved from palette.h
extern struct FromPalette block_info;

Editor::Editor() { }

void Editor::show(Context &m_context) {
    ImNodes::BeginNodeEditor();

    // Adding a node by pressing "A" keyboard
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        ImNodes::IsEditorHovered() && ImGui::IsKeyReleased(SDL_SCANCODE_A)) 
    {
        m_context.update(true, "");
    }

    // Adding a node by clicking on palette
    if (block_info.clicked) 
    {
        m_context.update(true, block_info.block_name);
        block_info.clicked = false;
    }

    // Delete a node by pressing "D" or "delete" keyboard
    int nodeid = isBlockClicked();
    auto string_nodeid = std::to_string(nodeid);
    if (nodeid != 0 && (ImGui::IsKeyReleased(SDL_SCANCODE_D) || ImGui::IsKeyReleased(SDL_SCANCODE_DELETE))) 
    {
        m_context.update(false, string_nodeid);
    }

    displayInEditor(m_context);

    ImNodes::MiniMap(0.1f, ImNodesMiniMapLocation_BottomRight);
    ImNodes::EndNodeEditor();
    Editor::showPopup();
}

void Editor::displayInEditor(Context m_context) {
    for (Block& block : m_context._blocks) {
        block.show();
    } 
    for (const Link& link : m_context._links) {
        ImNodes::Link(link.id, link.start_attr, link.end_attr);
    } 
}

void Editor::showPopup() {
    int nodeid;
    if (ImNodes::IsNodeHovered(&nodeid) && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup("my popup");
    }
    if (ImGui::BeginPopup("my popup")) {
        if (ImGui::MenuItem("Bypass")) {
            printf("Bypass\n");
        } 
        if (ImGui::MenuItem("Other")) {
            printf("Other\n");
        }
        ImGui::EndPopup();
    }
    
}

int Editor::isBlockClicked() {
    int nodeid;
    ImNodes::GetSelectedNodes(&nodeid);
    if (ImNodes::IsNodeSelected(nodeid)) {
        return nodeid;
    }
    return 0;
}

