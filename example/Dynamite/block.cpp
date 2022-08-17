#include "block.h"

#include <iterator>
#include <imgui.h>
#include <imnodes.h>

using namespace std;

Block::Block() {
    id = 0;
    type = "Block";
}

Block::Block(const int i) {
    id = i;
    type = "Block";
}

Block::Block(const int i, std::string n) {
    id = i;
    type = n;
}

int Block::getID() {
    return this->id;
}

std::string Block::getName() {
    return this->name;
}

std::string Block::getType() {
    return this->type.c_str();
}

int Block::getNumInputs() {
    return (int)this->_inPorts.size();
}

int Block::getNumOutputs() {
    return (int)this->_outPorts.size();
}

void Block::show() {
    // Set Input and Output blocks' node colors to red/orange
    if (type == "input" || type =="output") {
        ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(175, 41, 0, 255));
        ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(255, 127, 80, 255));
        ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(255, 127, 80, 255));
    }
    ImNodes::BeginNode(id);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(name.c_str());
    ImGui::NewLine();
    ImGui::TextUnformatted(type.c_str());
    ImNodes::EndNodeTitleBar();

    map<int,Port>::iterator itr;
    // Display input port names on the block
    for (itr = _inPorts.begin(); itr != _inPorts.end(); itr++) {
        ImNodes::BeginInputAttribute(itr->first);
        ImGui::TextUnformatted(itr->second.reference_name);
        ImNodes::EndInputAttribute();
    }

    // Display output port names on the block
    for (itr = _outPorts.begin(); itr != _outPorts.end(); itr++) {
        ImNodes::BeginOutputAttribute(itr->first);
        const float text_width = ImGui::CalcTextSize(itr->second.name).x;
        if (type != "input") {
            ImGui::Indent(120.f + ImGui::CalcTextSize("value").x - text_width);
        }
        ImGui::TextUnformatted(itr->second.name);
        ImNodes::EndOutputAttribute(); 
    }

    ImNodes::EndNode();
    if (type == "input" || type =="output") {
        ImNodes::PopColorStyle();
        ImNodes::PopColorStyle();
        ImNodes::PopColorStyle();
    }
}

void Block::bypass() {
    // gets called by block -> pop up -> edit menu
}

void Block::setName(std::string n) {
    this->name = n;
}

void Block::addInPort(int current_port_id, Port p) {
    _inPorts.insert({current_port_id, p});
}

void Block::addOutPort(int current_port_id, Port p) {
    _outPorts.insert({current_port_id, p});
}

void Block::addParam(int current_param_id, Parameter p) {
    _parameters.insert({current_param_id, p});
}

void Block::deleteInPort(int portid) {
    if (ImNodes::IsPinHovered(&portid) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        _inPorts.erase(portid); 
    }
}

void Block::deleteOutPort(int portid) {
    if (ImNodes::IsPinHovered(&portid) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        _outPorts.erase(portid); 
    }
}

// void Block::deleteBlock(Block block) {

// }
