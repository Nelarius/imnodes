#include "block.h"

Block::Block() {
    id = 0; numInCh = 2; numOutCh = 2;
    name = "Block"; input = "IN"; output = "OUT";
}

Block::Block(const int i) {
    id = i; numInCh = 2; numOutCh = 2;
    name = "Block"; input = "IN"; output = "OUT";
}

int Block::getID() {
    return this->id;
}
string Block::getName() {
    return this->name;
}
string Block::getInput() {
    return this->input;
}
string Block::getOutput() {
    return this->output;
}

void Block::show() {
    ImNodes::BeginNode(id);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(name.c_str());
    ImNodes::EndNodeTitleBar();

    for (int i = 0; i < numInCh; i++) {
        //const int attr_id = ++current_attr_id;
        ImNodes::BeginInputAttribute(i<<8);
        ImGui::TextUnformatted(input.c_str());
        ImNodes::EndInputAttribute();
    }

    for (int i = 0; i < numOutCh; i++) {
        //const int attr_id = ++current_attr_id;
        ImNodes::BeginOutputAttribute(i<<24);
        const float text_width = ImGui::CalcTextSize(output.c_str()).x;
        ImGui::Indent(120.f + ImGui::CalcTextSize("value").x - text_width);
        ImGui::TextUnformatted(output.c_str());
        ImNodes::EndOutputAttribute(); 
    }
    ImNodes::EndNode();
}

void Block::bypass() {
    // gets called by block -> pop up -> edit menu
}

// void Block::deleteBlock(Block block) {

// }
