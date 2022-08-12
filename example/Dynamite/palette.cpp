#include "palette.h"
#include <imgui.h>
#include <vector>
#include <string>
#include "context.h"

#include <iostream>

using namespace std;

// Retrieved from context.h
extern struct BlockNames names;

struct FromPalette block_info;

Palette::Palette() 
{
    // do nothing
}

void Palette::init()
{
    // Setting the style of palette components
    ImGuiStyle* style = &ImGui::GetStyle();

    style->WindowBorderSize = 0;

    style->Colors[ImGuiCol_Button] = ImColor(31, 30, 31, 255);
    style->Colors[ImGuiCol_ButtonActive] = ImColor(41, 40, 41, 255);
    style->Colors[ImGuiCol_ButtonHovered] = ImColor(41, 40, 41, 255);

    style->Colors[ImGuiCol_Separator] = ImColor(70, 70, 70, 255);
    style->Colors[ImGuiCol_SeparatorActive] = ImColor(76, 76, 76, 255);
    style->Colors[ImGuiCol_SeparatorHovered] = ImColor(76, 76, 76, 255);

    style->Colors[ImGuiCol_FrameBg] = ImColor(37, 36, 37, 255);
    style->Colors[ImGuiCol_FrameBgActive] = ImColor(37, 36, 37, 255);
    style->Colors[ImGuiCol_FrameBgHovered] = ImColor(37, 36, 37, 255);
}

void Palette::drawBlockBrowser(Blocks contents)
{
    ImGui::TextUnformatted("Block Browser");
    // ImGui::NewLine();
    static ImGuiTextFilter filter;
    filter.Draw("##Search", ImGui::GetContentRegionAvail().x); // Need to fix inputTextHint in imgui.cpp

    static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_DefaultOpen;

    int block_types_len = static_cast<int>(contents.block_types.size());
    for (int i = 0; i < block_types_len; i++)
    {
        ImGuiTreeNodeFlags node_flags = base_flags;
        // Tree node for block types
        bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)i, node_flags, contents.block_types[i].c_str(), i);

        if (node_open)
        {
            // Leaf node for io blocks
            if (contents.block_types[i] == "IO Blocks") 
            {
                for (const auto& io_block : contents.io_blocks)
                {
                    if (filter.PassFilter(io_block.c_str())) // Search bar filter for io blocks
                    {
                        node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                        ImGui::TreeNodeEx(io_block.c_str(), node_flags);
                        if (ImGui::IsItemClicked()) 
                        {
                            if ((!block_info.input_placed && io_block == "input")) {
                                block_info.clicked = true;
                                block_info.block_type = io_block;
                                block_info.input_placed = true;
                            }
                            if ((!block_info.output_placed && io_block == "output")) {
                                block_info.clicked = true;
                                block_info.block_type = io_block;
                                block_info.output_placed = true;
                            }
                        }
                        if (ImGui::BeginDragDropSource())
                        {
                            ImGui::SetDragDropPayload(io_block.c_str(), NULL, 0);
                            ImGui::Text("%s", io_block.c_str());
                            ImGui::EndDragDropSource();
                        }
                    }
                }
            }
            // Leaf node for dsp blocks
            else if (contents.block_types[i] == "DSP Blocks")
            {
                for (const auto& dsp_block : contents.dsp_blocks) 
                {
                    if (filter.PassFilter(dsp_block.c_str())) // Search bar filter for dsp blocks
                    {
                        node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                        ImGui::TreeNodeEx(dsp_block.c_str(), node_flags);
                        if (ImGui::IsItemClicked()) 
                        {
                            block_info.clicked = true;
                            block_info.block_type = dsp_block;
                        }
                        if (ImGui::BeginDragDropSource())
                        {
                            ImGui::SetDragDropPayload(dsp_block.c_str(), NULL, 0);
                            ImGui::Text("%s", dsp_block.c_str());
                            ImGui::EndDragDropSource();
                        }
                    }
                }
            }
            // Leaf node for control blocks
            else 
            {
                for (const auto& control_block : contents.control_blocks)
                {
                    if (filter.PassFilter(control_block.c_str())) // Search bar filter for control blocks
                    {
                        node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                        ImGui::TreeNodeEx(control_block.c_str(), node_flags);
                        if (ImGui::IsItemClicked()) 
                        {
                            block_info.clicked = true;
                            block_info.block_type = control_block;
                        }
                        if (ImGui::BeginDragDropSource())
                        {
                            ImGui::SetDragDropPayload(control_block.c_str(), NULL, 0);
                            ImGui::Text("%s", control_block.c_str());
                            ImGui::EndDragDropSource();
                        }
                    }
                }
            }
            ImGui::TreePop();
        }
    }
}

void Palette::show() 
{
    // Left side
    ImGui::Columns(2);
    ImGui::SetColumnOffset(1, 50);

    // Rendering a gray vertical rectangle
    const float palette_width = 60;
    ImDrawList* drawListR = ImGui::GetWindowDrawList();
    drawListR->AddRectFilled(ImVec2(0, 0), ImVec2(palette_width, ImGui::GetIO().DisplaySize.y), IM_COL32(51, 51, 51, 255));

    static int tab = 0;

    ImGui::NewLine();
    // Rendering all the buttons in a column
    ImGui::PushStyleColor(ImGuiCol_Button, tab == 1 ? IM_COL32(41, 40, 41, 255) : IM_COL32(31, 30, 31, 255));
    if (ImGui::Button("DSP", ImVec2(50 - 15, 40)))
    {
        tab = 1;
    }
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Button, tab == 2 ? IM_COL32(41, 40, 41, 255) : IM_COL32(31, 30, 31, 255));
    if (ImGui::Button("SYS", ImVec2(50 - 15, 40)))
    {
        tab = 2;
    }
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Button, tab == 3 ? IM_COL32(41, 40, 41, 255) : IM_COL32(31, 30, 31, 255));
    if (ImGui::Button("OUT", ImVec2(50 - 15, 40)))
    {
        tab = 3;
    }
    ImGui::SetCursorPosY(ImGui::GetIO().DisplaySize.y * 0.9f);
    ImGui::PushStyleColor(ImGuiCol_Button, tab == 4 ? IM_COL32(41, 40, 41, 255) : IM_COL32(31, 30, 31, 255));
    if (ImGui::Button("SET", ImVec2(50 - 15, 40)))
    {
        tab = 4;
    }

    ImGui::PopStyleColor(4);
    
    // Right side
    ImGui::NextColumn();
    
    // Use this switch statement for future palette button click actions
    switch (tab)
    {
        case 1: {
            // Render block browser
            static std::vector<std::string> block_types;
            static std::vector<std::string> io_blocks;
            static std::vector<std::string> dsp_blocks;
            static std::vector<std::string> control_blocks;
            if (tab == 1 && block_types.empty())
            {
                block_types = { "IO Blocks", "DSP Blocks", "Control Blocks" };
                io_blocks = { "input", "output" };
                dsp_blocks = names.dsp_names;
                control_blocks = names.control_names;
            }

            // Blocks list initialization
            struct Blocks contents;
            contents.block_types = block_types;
            contents.io_blocks = io_blocks;
            contents.dsp_blocks = dsp_blocks;
            contents.control_blocks = control_blocks;

            // Render list of blocks
            drawBlockBrowser(contents);
            break;
        }

        case 2: {
            system_clicked = true;
            break;
        }
    }
}

void Palette::exit() 
{
    // do nothing
}
