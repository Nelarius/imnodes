#include "editor.h"
#include <string>

#include "palette.h"
// Retrieved from palette.h
extern struct FromPalette block_info;

static bool add_in_port = false;
static bool add_out_port = false;

static void addBlockInPort(Context &m_context, int id);
static void addBlockOutPort(Context &m_contect, int id);

Editor::Editor() { }

void Editor::show(Context &m_context) {
    ImNodes::BeginNodeEditor();

    // Adding a node by pressing "A" keyboard
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        ImNodes::IsEditorHovered() && ImGui::IsKeyReleased(SDL_SCANCODE_A)) 
    {
        m_context.update(true, "Block");
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
    Editor::showPopup(m_context);

    deletePort(m_context);
}

void Editor::displayInEditor(Context m_context) {
    for (Block& block : m_context._blocks) {
        block.show();
    } 
    for (const Link& link : m_context._links) {
        ImNodes::Link(link.id, link.start_attr, link.end_attr);
    } 
}

void Editor::showPopup(Context &m_context) {
    int nodeid;

    if (ImNodes::IsNodeHovered(&nodeid) && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) { 
        ImGui::OpenPopup("my popup"); 
    }
    if (add_in_port) addBlockInPort(m_context, nodeid);
    if (add_out_port) addBlockOutPort(m_context, nodeid);

    if (ImGui::BeginPopup("my popup")) {
        if (ImGui::MenuItem("Bypass")) { printf("Bypass\n"); } // link to block.bypass()
        ImGui::MenuItem("Add Channel-In", NULL, &add_in_port);
        ImGui::MenuItem("Add Channel-Out", NULL, &add_out_port);
        
        ImGui::EndPopup();
    }
}

static void addBlockInPort(Context &m_context, int nodeid) {
    for (auto i = 0; i < (int)m_context._blocks.size(); i++) {
        if (m_context._blocks[i].getID() == nodeid) {
            Port port(m_context.current_port_id, "INPUT");
            m_context._blocks[i].addInPort(m_context.current_port_id, port);
            ++m_context.current_port_id;
        }
    }
    add_in_port = false;
}

static void addBlockOutPort(Context &m_context, int nodeid) {
    for (auto i = 0; i < (int)m_context._blocks.size(); i++) {
        if (m_context._blocks[i].getID() == nodeid) {
            Port port(m_context.current_port_id, "OUTPUT");
            m_context._blocks[i].addOutPort(m_context.current_port_id, port);
            ++m_context.current_port_id;
        }
    }
    add_out_port = false; 
}

void Editor::deletePort(Context &m_context) {
    int portid = 0;
    for (Block& block : m_context._blocks) {
        block.deleteInPort(portid);
        block.deleteOutPort(portid);
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

