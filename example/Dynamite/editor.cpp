#include "editor.h"
#include <string>
#include <SDL_scancode.h>

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
    // This can be deleted as it is not being used
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        ImNodes::IsEditorHovered() && ImGui::IsKeyReleased(SDL_SCANCODE_A)) 
    {
        m_context.update(true, "Block");
    }

    // Adding a node by clicking on palette
    if (block_info.clicked) 
    {
        m_context.update(true, block_info.block_type);
        block_info.clicked = false;
    }

    // Delete a node by pressing "delete" or "backspace" keyboard
    int nodeid = isBlockClicked();
    auto string_nodeid = std::to_string(nodeid);
    if (nodeid != 0 && ImNodes::IsEditorHovered() &&
        (ImGui::IsKeyReleased(SDL_SCANCODE_DELETE) || ImGui::IsKeyReleased(SDL_SCANCODE_BACKSPACE))) 
    {
        static string current_block_type;
        for (auto i = 0; i < (int)m_context._blocks.size(); i++) 
        {
            if (m_context._blocks[i].getID() == nodeid) {
                current_block_type = m_context._blocks[i].getType();
            }
        }

        if (current_block_type == "input")
        {
            block_info.input_placed = false;
        }
        else if (current_block_type == "output") 
        {
            block_info.output_placed = false;
        }

        m_context.update(false, string_nodeid);
    }

    // Clear the editor by pressing "C" keyboard
    static bool messagebox = false;
    if (ImNodes::IsEditorHovered() && ImGui::IsKeyReleased(SDL_SCANCODE_C)) 
    {
        messagebox = true;
    }
    if (messagebox) {
        ImGui::OpenPopup("Clear");

        // Always center this window when appearing
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Clear", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Are you sure you want to delete the system?\nThis operation cannot be undone!\n\n");
            ImGui::Separator();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            ImGui::PopStyleVar();

            if (ImGui::Button("OK", ImVec2(150, 0))) { 
                // Clearing blocks and links
                m_context._blocks.clear();
                block_info.input_placed = false;
                block_info.output_placed = false;

                m_context._links.clear();

                messagebox = false;
                ImGui::CloseCurrentPopup(); 
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(150, 0))) {
                messagebox = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    // Displays blocks and links in the editor
    displayInEditor(m_context);

    // Displays the minimap
    ImNodes::MiniMap(0.1f, ImNodesMiniMapLocation_BottomRight);

    ImNodes::EndNodeEditor();
    Editor::showPopup(m_context);

    // Check if there are triggers to delete a port
    // Revisit this when refactoring
    // This is related to the case of adding and deleting ports
    // ui.cpp Line 128
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
    static int nodeid;

    if (ImNodes::IsNodeHovered(&nodeid) && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) { 
        ImGui::OpenPopup("my popup"); 
    }
    if (add_in_port) {
        addBlockInPort(m_context, nodeid);
    }
    if (add_out_port) {
        addBlockOutPort(m_context, nodeid);
    }

    // Find out which block is getting right clicked
    static string current_block_type;
    for (auto i = 0; i < (int)m_context._blocks.size(); i++) {
        if (m_context._blocks[i].getID() == nodeid) {
            current_block_type = m_context._blocks[i].getType();
        }
    }

    if (ImGui::BeginPopup("my popup")) {
        if (ImGui::MenuItem("Bypass")) { printf("Bypass\n"); } // Link to block.bypass()
        if (current_block_type == "input") {
            ImGui::MenuItem("Add Channel-Out", NULL, &add_out_port);
        }
        else if (current_block_type == "output") {
            ImGui::MenuItem("Add Channel-In", NULL, &add_in_port);
        }
        else {
            ImGui::MenuItem("Add Channel-In", NULL, &add_in_port);
            ImGui::MenuItem("Add Channel-Out", NULL, &add_out_port);
        }
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