#include "multipanel.h"
#include "context.h"
#include <map>
#include <iterator>
#include <string>

#include <iostream>

using namespace std;

// Retrieved from context.h
extern struct BlockParameters parameters;

MultiPanel::MultiPanel() {
    // do nothing
}

void MultiPanel::init() {
    // do nothing
}

void MultiPanel::show(Editor& m_editor, Context& m_context) {
    // Flag allowing the tabs to be reorderable
    static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_Reorderable;

    // Tab Bar
    // what are we using each of these tabs for? and where will block info go

    const char* tabNames[3] = { "Inspector", "Output", "Terminal" };
    static bool opened[3] = { true, true, true }; // Persistent user state
    // Checkbox for checking if tab closed or not (remove in the future)
    // for (int n = 0; n < IM_ARRAYSIZE(opened); n++)
    // {
    //     if (n > 0) 
    //     { 
    //         ImGui::SameLine(); 
    //     }
    //     ImGui::Checkbox(tabNames[n], &opened[n]);
    // }

    // Underlying bool* will be set to false when the tab is closed
    if (ImGui::BeginTabBar("MultiTabBar", tab_bar_flags))
    {
        for (int n = 0; n < IM_ARRAYSIZE(opened); n++) 
        {

            if (opened[n] && ImGui::BeginTabItem(tabNames[n], &opened[n], ImGuiTabItemFlags_None))
            {
                ImGui::NewLine();
                // DISPLAY BLOCK INFO HERE
                MultiPanel::showBlockInfo(m_editor, m_context);
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
}

void MultiPanel::exit() {
    // do nothing
}

void MultiPanel::showBlockInfo(Editor& editor, Context& context) {
    int nodeid = editor.isBlockClicked();
    if (nodeid != 0) {
        for (Block& block : context._blocks) {
            if (nodeid == block.getID()) {
                MultiPanel::formatInfo(block);
            }
        }
    }
}

void MultiPanel::formatInfo(Block& block) {
    // Block type
    ImGui::TextUnformatted("Block type: ");
    ImGui::SameLine();
    ImGui::TextUnformatted(block.getType().c_str());

    // Block name
    ImGui::TextUnformatted("Block name: ");
    ImGui::SameLine();
    static char blockname_field[40] = "";
    // Check if block name already exists
    if (block.getName() != "") {
        std::strcpy(blockname_field, block.getName().c_str());
    }
    else {
        std::strcpy(blockname_field, "");
    }
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.2);
    auto flag = ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_AutoSelectAll;
    ImGui::InputText("##BlockName", blockname_field, IM_ARRAYSIZE(blockname_field), flag, MultiPanelFuncs::blockNameCallBack, (void *)&block);
    ImGui::PopItemWidth();

    // Block input/output and parameters
    static ImGuiTableFlags table_flags = ImGuiTableFlags_BordersInnerV; 
    if (ImGui::BeginTable("block info table", 3, table_flags)) {
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Input:");
        ImGui::Indent();

        int channel_index = 1;

        // Input channels name text field generation
        for (const auto &itr : block._inPorts) {
            std::string numberedInput = "INPUT ";
            numberedInput.append(std::to_string(channel_index));
            ImGui::TextUnformatted(numberedInput.c_str());
            ++channel_index;
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5);
            std::string textFieldID = "##InputName";
            textFieldID.append(std::to_string(itr.first));
            char* buf = (char*)itr.second.name;
            ImGui::InputText(textFieldID.c_str(), buf, 40);
            ImGui::PopItemWidth();
        }
        
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Output:");
        ImGui::Indent();

        // Reset index for output ports
        channel_index = 1;

        // Output channels name text field generation
        for (const auto &itr : block._outPorts) {
            std::string numberedOutput = "OUTPUT ";
            numberedOutput.append(std::to_string(channel_index));
            ImGui::TextUnformatted(numberedOutput.c_str());
            ++channel_index;
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5);
            std::string textFieldID = "##OutputName";
            textFieldID.append(std::to_string(itr.first));
            char* buf = (char*)itr.second.name;
            ImGui::InputText(textFieldID.c_str(), buf, 40);
            ImGui::PopItemWidth();
        }

        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Parameters:");
        ImGui::Indent();

        // Call to retrieve block parameters
        for (const auto &itr : block._parameters)
        {
            ImGui::TextUnformatted((itr.second.name).c_str());
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5);
            std::string textFieldID = "##Parameter";
            textFieldID.append(std::to_string(itr.first));
            char* buf = (char*)itr.second.value;
            ImGui::InputTextWithHint(textFieldID.c_str(), (itr.second.type).c_str(), buf, 40);
            ImGui::PopItemWidth();
        }

        ImGui::EndTable();
    }
}