#include "block.h"

void show(Block block) {
    ImNodes::BeginNode(block.id);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(block.name);
    ImNodes::EndNodeTitleBar();

    ImNodes::BeginInputAttribute(block.id << 8);
    ImGui::TextUnformatted(block.input);
    ImNodes::EndInputAttribute();

    ImNodes::BeginOutputAttribute(block.id << 24);
    const float text_width = ImGui::CalcTextSize(block.output).x;
    ImGui::Indent(120.f + ImGui::CalcTextSize("value").x - text_width);
    ImGui::TextUnformatted(block.output);
    ImNodes::EndOutputAttribute();  

    ImNodes::EndNode();
}

void bypass(); // gets called by block -> pop up -> edit menu

// need to delete it from the editor context
void deleteBlock(Block block) {

}