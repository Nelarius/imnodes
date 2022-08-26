#include "editor.h"
#include <string>
#include <SDL_scancode.h>

#include "palette.h"
#include "graph.h"

// Retrieved from palette.h
extern struct FromPalette block_info;

static bool add_in_port = false;
static bool add_out_port = false;

static void addBlockInPort(Graph &graph, int id);
static void addBlockOutPort(Graph &graph, int id);

Editor::Editor() { }

void Editor::show(Context &m_context) {
    ImNodes::BeginNodeEditor();
    Graph &graph = m_context.m_graph;

    // Adding a node by pressing "A" keyboard
    // This can be deleted as it is not being used
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        ImNodes::IsEditorHovered() && ImGui::IsKeyReleased(SDL_SCANCODE_A)) 
    {
        graph.addBlock("Block");
    }

    // Adding a node by clicking on palette
    if (block_info.clicked) 
    {
        graph.addBlock(block_info.block_type);
        block_info.clicked = false;
    }

    // Delete a node by pressing "delete" or "backspace" keyboard
    int node_id = isBlockClicked();
    //auto string_nodeid = std::to_string(node_id);
    if (node_id != 0 && ImNodes::IsEditorHovered() &&
        (ImGui::IsKeyReleased(SDL_SCANCODE_DELETE) || ImGui::IsKeyReleased(SDL_SCANCODE_BACKSPACE))) 
    {
        static string current_block_type;
        for (auto i = 0; i < (int)graph._blocks.size(); i++) {
            if (graph._blocks[i].getID() == node_id) {
                current_block_type = graph._blocks[i].getType();
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

        graph.deleteBlock(node_id);
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
                //graph._blocks.clear();
                graph.clearBlocks();
                block_info.input_placed = false;
                block_info.output_placed = false;
                graph.clearLinks();
                //m_context._links.clear();

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

    displayInEditor(graph);

    // Displays the minimap
    ImNodes::MiniMap(0.1f, ImNodesMiniMapLocation_BottomRight);

    ImNodes::EndNodeEditor();
    Editor::showPopup(graph);

    deletePort(graph);
}

void Editor::displayInEditor(Graph graph) {
    for (Block& block : graph._blocks) {
        block.show();
    } 
    for (const Link& link : graph._links) {
        ImNodes::Link(link.id, link.start_attr, link.end_attr);
    } 
}

void Editor::showPopup(Graph &graph) {
    static int node_id;

    if (ImNodes::IsNodeHovered(&node_id) && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) { 
        ImGui::OpenPopup("my popup"); 
    }
    if (add_in_port) addBlockInPort(graph, node_id);
    if (add_out_port) addBlockOutPort(graph, node_id);

    // Find out which block is getting right clicked
    static string current_block_type;
    for (auto i = 0; i < (int)graph._blocks.size(); i++) {
        if (graph._blocks[i].getID() == node_id) {
            current_block_type = graph._blocks[i].getType();
        }
    }

    if (ImGui::BeginPopup("my popup")) {
        if (ImGui::MenuItem("Bypass")) { printf("Bypass\n"); } // link to block.bypass()
        if (current_block_type != "input") {
            ImGui::MenuItem("Add Channel-In", NULL, &add_in_port);
        } else if (current_block_type != "output") {
            ImGui::MenuItem("Add Channel-Out", NULL, &add_out_port);
        }

        /*
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
        //*/
        ImGui::EndPopup();
    }
}

static void addBlockInPort(Graph &graph, int nodeid) {
    for (auto i = 0; i < (int)graph._blocks.size(); i++) {
        if (graph._blocks[i].getID() == nodeid) {
            Port port(graph.current_port_id, "INPUT");
            graph._blocks[i].addInPort(graph.current_port_id, port);
            ++graph.current_port_id;
        }
    }
    add_in_port = false;
}

static void addBlockOutPort(Graph &graph, int nodeid) {
    for (auto i = 0; i < (int)graph._blocks.size(); i++) {
        if (graph._blocks[i].getID() == nodeid) {
            Port port(graph.current_port_id, "OUTPUT");
            graph._blocks[i].addOutPort(graph.current_port_id, port);
            ++graph.current_port_id;
        }
    }
    add_out_port = false; 
}

void Editor::deletePort(Graph &graph) {
    int portid = 0;
    for (Block& block : graph._blocks) {
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