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

void MultiPanel::show(Editor& m_editor, Palette& m_palette, Context& m_context) {
    // Flag allowing the tabs to be reorderable
    static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs;

    // Tab Bar
    // what are we using each of these tabs for? and where will block info go
    const char* tabNames[4] = { "Inspector", "System Configuration", "Output", "Terminal" };
    static bool opened[4] = { true, false, true, true }; // Persistent user state

    // Show 'Inspector' tab
    if (m_palette.block_clicked) {
        opened[0] = true;
        m_palette.block_clicked = false;
    }

    // Show 'System Configuration' Tab
    if (m_palette.system_clicked) {
        opened[1] = true;
        m_palette.system_clicked = false;
    }

    // Underlying bool* will be set to false when the tab is closed
    if (ImGui::BeginTabBar("MultiTabBar", tab_bar_flags))
    {
        for (int n = 0; n < IM_ARRAYSIZE(opened); n++) 
        {
            if (opened[n] && ImGui::BeginTabItem(tabNames[n], &opened[n], ImGuiTabItemFlags_None))
            {
                ImGui::NewLine();
                // Display block attributes
                if (n == 0) {
                    MultiPanel::showBlockInfo(m_editor, m_context);
                }
                else if (n == 1) {
                    MultiPanel::showSystemInfo(m_context);
                }
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
}

void MultiPanel::exit() {
    // do nothing
}

void MultiPanel::showSystemInfo(Context& m_context) {
    // System name
    ImGui::TextUnformatted("System name: ");
    ImGui::SameLine();
    static char systemname_field[40] = "";
    // Check if system name already exists
    if (m_context.system_name != "") {
        std::strcpy(systemname_field, (m_context.system_name).c_str());
    }
    else {
        std::strcpy(systemname_field, "");
    }
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.2);
    auto flag = ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_AutoSelectAll;
    ImGui::InputText("##SystemName", systemname_field, 40, flag, MultiPanelFuncs::systemNameCallBack, (void *)&m_context);
    ImGui::PopItemWidth();

    ImGui::NewLine();

    static ImGuiTableFlags table_flags = ImGuiTableFlags_BordersInnerV; 
    if (ImGui::BeginTable("system info", 3, table_flags)) {
        ImGui::TableNextColumn();
        // Target player IP address
        ImGui::TextUnformatted("Player IP address: ");
        ImGui::SameLine();
        static char ip_field[40] = "";
        // Check if ip address already exists
        if (m_context.target_ip_address != "") {
            std::strcpy(ip_field, (m_context.target_ip_address).c_str());
        }
        else {
            std::strcpy(ip_field, "");
        }
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5);
        ImGui::InputText("##IPAddress", ip_field, 40, flag, MultiPanelFuncs::ipAddressCallBack, (void *)&m_context);
        ImGui::PopItemWidth();

        // Enable chirp and Trueplay (no functionality yet)
        ImGui::TableNextColumn();
        ImGui::Checkbox("Enable Chirp", &m_context.chirp_enabled);
        ImGui::Checkbox("Enable Trueplay", &m_context.trueplay_enabled);

        // Serialize to JSON or Protobuf (no functionality yet)
        ImGui::TableNextColumn();
        ImGui::Checkbox("Serialize in Protobuf", &m_context.serialize_protobuf);

        ImGui::EndTable();
    }
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
        // Future potential edit:
        // If we want input port names to not cascade other block's output port names and let it have its own name,
        // delete the three lines below and the port class will utilize its own "name" member variable
        // this will allow the port to have its name and not have it assigned to the input port name's reference.
        for (auto &itr : block._inPorts) {
            std::string numberedInput = "INPUT ";
            numberedInput.append(std::to_string(channel_index));
            ImGui::TextUnformatted(numberedInput.c_str());
            ++channel_index;
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5);
            std::string textFieldID = "##InputName";
            textFieldID.append(std::to_string(itr.first));
            char* buf = (char*)itr.second.name;
            if (itr.second.reference_name != nullptr) {
                buf = itr.second.reference_name;
            }
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
            if (itr.second.name != "none") { // Don't render when there are no parameters
                ImGui::TextUnformatted((itr.second.name).c_str());
                ImGui::SameLine();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5);
                std::string textFieldID = "##Parameter";
                textFieldID.append(std::to_string(itr.first));
                char* buf = (char*)itr.second.value;
                ImGui::InputTextWithHint(textFieldID.c_str(), (itr.second.type).c_str(), buf, 40);
                ImGui::PopItemWidth();
            }
        }

        ImGui::EndTable();
    }
}