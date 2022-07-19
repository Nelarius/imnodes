#include "block.h"

Block::Block() {
    id = 0; numInCh = 2; numOutCh = 2;
    name = "Block"; input = "IN"; output = "OUT";
}

Block::Block(const int i) {
    id = i; numInCh = 2; numOutCh = 2;
    name = "Block"; input = "IN"; output = "OUT";
}

void Block::show() {
    ImNodes::BeginNode(id);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(name.c_str());
    ImNodes::EndNodeTitleBar();

    ImNodes::BeginInputAttribute(id << 8);
    ImGui::TextUnformatted(input.c_str());
    ImNodes::EndInputAttribute();

    ImNodes::BeginOutputAttribute(id << 24);
    const float text_width = ImGui::CalcTextSize(output.c_str()).x;
    ImGui::Indent(120.f + ImGui::CalcTextSize("value").x - text_width);
    ImGui::TextUnformatted(output.c_str());
    ImNodes::EndOutputAttribute();  

    ImNodes::EndNode();
}

void Block::bypass() {
    // gets called by block -> pop up -> edit menu
}

// void Block::deleteBlock(Block block) {

// }
