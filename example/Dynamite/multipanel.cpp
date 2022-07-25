#pragma once

#include "multipanel.h"
using namespace std;

MultiPanel::MultiPanel() {
    // do nothing
}

void MultiPanel::init() {
    // do nothing
}

void MultiPanel::show(Editor m_editor, Context m_context) {
    // Setting the display location of Multipanel
    /*
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.2f, ImGui::GetIO().DisplaySize.y * 0.7f));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y), 2);
    ImGui::SetNextWindowSizeConstraints(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y * 0.3f), ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y));
    */
    //ImGui::Begin("Multipanel");

    // Flag allowing the tabs to be reorderable
    static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_Reorderable;

    // Tab Bar
    // what are we using each of these tabs for? and where will block info go
    const char* names[3] = { "Inspector", "Output", "Terminal" };
    static bool opened[3] = { true, true, true }; // Persistent user state
    for (int n = 0; n < IM_ARRAYSIZE(opened); n++) // Checkbox for checking if tab closed or not (remove in the future)
    {
        if (n > 0) 
        { 
            ImGui::SameLine(); 
        }
        ImGui::Checkbox(names[n], &opened[n]);
    }

    // Underlying bool* will be set to false when the tab is closed
    if (ImGui::BeginTabBar("MultiTabBar", tab_bar_flags))
    {
        for (int n = 0; n < IM_ARRAYSIZE(opened); n++) 
        {

            if (opened[n] && ImGui::BeginTabItem(names[n], &opened[n], ImGuiTabItemFlags_None))
            {
                ImGui::Text("This is the %s tab!", names[n]);
                ImGui::NewLine();
                /* DISPLAY BLOCK INFO HERE */
                MultiPanel::showBlockInfo(m_editor, m_context);
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
    //ImGui::End();
}

void MultiPanel::exit() {
    // do nothing
}

void MultiPanel::showBlockInfo(Editor m_editor, Context m_context) {
    int nodeid = m_editor.isBlockHovered();
    if (nodeid != 0) {
        for (Block& block : m_context._blocks) {
            if (nodeid == block.getID()) {
                MultiPanel::formatInfo(block);
            }
        } 
    }
}

void MultiPanel::formatInfo(Block block) {
    ImGui::TextUnformatted(block.getName().c_str());
    static ImGuiTableFlags table_flags = ImGuiTableFlags_BordersInnerV; 
    if (ImGui::BeginTable("block info table", 3, table_flags)) {
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Input:");
        ImGui::Indent();
        ImGui::TextUnformatted(block.getInput().c_str());
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Output:");
        ImGui::Indent();
        ImGui::TextUnformatted(block.getOutput().c_str());
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Parameters:");
        ImGui::EndTable();
    }
}