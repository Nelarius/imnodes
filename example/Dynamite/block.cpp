#include "block.h"

Block::Block() {
    id = 0; name = "Block"; input = "IN"; output = "OUT";
    param1 = NULL; param2 = 0.0f; 
}

void Block::show() {
    ImNodes::BeginNode(id);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(name);
    ImNodes::EndNodeTitleBar();

    ImNodes::BeginInputAttribute(id << 8);
    ImGui::TextUnformatted(input);
    ImNodes::EndInputAttribute();

    ImNodes::BeginOutputAttribute(id << 24);
    const float text_width = ImGui::CalcTextSize(output).x;
    ImGui::Indent(120.f + ImGui::CalcTextSize("value").x - text_width);
    ImGui::TextUnformatted(output);
    ImNodes::EndOutputAttribute();  

    ImNodes::EndNode();
}

void Block::bypass() {
    // gets called by block -> pop up -> edit menu
}

// need to delete it from the editor context
/* void deleteBlock(Block block) {

} */
