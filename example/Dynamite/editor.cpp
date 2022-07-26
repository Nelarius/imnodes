#include "editor.h"

Editor::Editor() { }

void Editor::show(Context &m_context) {
    ImNodes::BeginNodeEditor();
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        ImNodes::IsEditorHovered() && ImGui::IsKeyReleased(SDL_SCANCODE_A)) {
        m_context.update();
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

int Editor::isBlockHovered() {
    int nodeid;
    if (ImNodes::IsNodeHovered(&nodeid)) {
        return nodeid;
    }
    return 0;
}

