#include "block.h"

Block::Block() {
    id = 0;
    name = "Block";
}

Block::Block(const int i) {
    id = i;
    name = "Block";
}

Block::Block(const int i, string n) {
    id = i;
    name = n;
}

int Block::getID() {
    return this->id;
}

string Block::getName() {
    return this->name;
}

int Block::getNumInputs() {
    return (int)this->_inPorts.size();
}

int Block::getNumOutputs() {
    return (int)this->_outPorts.size();
}

void Block::show() {
    ImNodes::BeginNode(id);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(name.c_str());
    ImNodes::EndNodeTitleBar();

    map<int,string>::iterator itr;
    for (itr = _inPorts.begin(); itr != _inPorts.end(); itr++) {
        ImNodes::BeginInputAttribute(itr->first);
        ImGui::TextUnformatted(itr->second.c_str());
        ImNodes::EndInputAttribute();
    }

    for (itr = _outPorts.begin(); itr != _outPorts.end(); itr++) {
        ImNodes::BeginOutputAttribute(itr->first);
        const float text_width = ImGui::CalcTextSize(itr->second.c_str()).x;
        ImGui::Indent(120.f + ImGui::CalcTextSize("value").x - text_width);
        ImGui::TextUnformatted(itr->second.c_str());
        ImNodes::EndOutputAttribute(); 
    }

    ImNodes::EndNode();
}

void Block::bypass() {
    // gets called by block -> pop up -> edit menu
}

void Block::addInPort(int current_port_id, string chan_name) {
    _inPorts[current_port_id] = chan_name;
}

void Block::addOutPort(int current_port_id, string chan_name) {
    _outPorts[current_port_id] = chan_name;
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
