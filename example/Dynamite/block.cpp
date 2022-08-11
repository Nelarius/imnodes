#include "block.h"
#include <iostream>

Block::Block() {
    id = 0;
    type = "Block";
}

Block::Block(const int i) {
    id = i;
    type = "Block";
}

Block::Block(const int i, string n) {
    id = i;
    type = n;
}

int Block::getID() {
    return this->id;
}

string Block::getName() {
    return this->name;
}

string Block::getType() {
    return this->type;
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
    ImGui::NewLine();
    ImGui::TextUnformatted(type.c_str());
    ImNodes::EndNodeTitleBar();

    map<int,Port>::iterator itr;
    for (itr = _inPorts.begin(); itr != _inPorts.end(); itr++) {
        ImNodes::BeginInputAttribute(itr->first);
        ImGui::TextUnformatted(itr->second.name);
        ImNodes::EndInputAttribute();
    }

    for (itr = _outPorts.begin(); itr != _outPorts.end(); itr++) {
        ImNodes::BeginOutputAttribute(itr->first);
        const float text_width = ImGui::CalcTextSize(itr->second.name).x;
        ImGui::Indent(120.f + ImGui::CalcTextSize("value").x - text_width);
        ImGui::TextUnformatted(itr->second.name);
        ImNodes::EndOutputAttribute(); 
    }

    ImNodes::EndNode();
}

void Block::bypass() {
    // gets called by block -> pop up -> edit menu
}

void Block::setName(string n) {
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
