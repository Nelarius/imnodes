#include "palette.h"
#include <imgui.h>
#include <vector>
#include <string>

using namespace std;

Palette::Palette() {
    // do nothing
}

void Palette::init() {
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

void Palette::show() {
    // Setting the display location of Palette
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, ImGui::GetFrameHeight() * 2), 1); // Need to justify the usage of GetFrameHeight for y coordinate
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x * 0.2, ImGui::GetIO().DisplaySize.y), 2);
    ImGui::SetNextWindowSizeConstraints(ImVec2(58, ImGui::GetIO().DisplaySize.y), ImVec2(ImGui::GetIO().DisplaySize.x * 0.9, ImGui::GetIO().DisplaySize.y));

    ImGui::Begin("Palette");

    // Left side
    ImGui::Columns(2);
    ImGui::SetColumnOffset(1, 50);

    // Rendering a gray vertical rectangle
    const float palette_width = 47;
    ImDrawList* drawListR = ImGui::GetWindowDrawList();
    drawListR->AddRectFilled(ImVec2(0, 0), ImVec2(palette_width, ImGui::GetIO().DisplaySize.y), IM_COL32(51, 51, 51, 255));

    static int tab = 0;

    // Rendering all the buttons in a column
    ImGui::PushStyleColor(ImGuiCol_Button, tab == 1 ? IM_COL32(41, 40, 41, 255) : IM_COL32(31, 30, 31, 255));
    if (ImGui::Button("DSP", ImVec2(50 - 15, 40)))
    {
        tab = 1;
    }
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Button, tab == 2 ? IM_COL32(41, 40, 41, 255) : IM_COL32(31, 30, 31, 255));
    if (ImGui::Button("Out", ImVec2(50 - 15, 40)))
    {
        tab = 2;
    }
    ImGui::SetCursorPosY(ImGui::GetIO().DisplaySize.y * 0.9);
    ImGui::PushStyleColor(ImGuiCol_Button, tab == 3 ? IM_COL32(41, 40, 41, 255) : IM_COL32(31, 30, 31, 255));
    if (ImGui::Button("Set", ImVec2(50 - 15, 40)))
    {
        tab = 3;
    }

    ImGui::PopStyleColor(3);
    
    // Right side
    ImGui::NextColumn();
    
    // Block browser tab
    static std::vector<std::string> block_browser;
    if (tab == 1 && block_browser.empty()) {
        block_browser.push_back("IO Blocks");
        block_browser.push_back("DSP Blocks");
        block_browser.push_back("Control Blocks");
    }

    // I/O blocks
    std::string io_list[] = 
    {
        "input",
        "output"
    };
    static std::vector<std::string> io_blocks(io_list, io_list + 2);

    // DSP blocks
    std::string dsp_list[] = 
    {
        "adaptive_limiter", 
        "array_sub_system", 
        "delay_line",
        "dot_product",
        "dynamic_hi_pass",
        "energy_sum_bass_manager",
        "excursion_control",
        "excursion_control_sub",
        "gain_fb",
        "iir",
        "limiter",
        "meter",
        "meter_exp",
        "meter_peak",
        "mixer",
        "multi_gain",
        "multi_therm_control",
        "port_noise_control",
        "power_source_control",
        "ramp_iir",
        "signal_generator",
        "smooth_multi_gain",
        "soft_clipper",
        "sonar_spatial_processing",
        "squancher",
        "thermal_estimator",
        "tone_control",
        "weighted_sum",
        "xfade_ramp_iir"
    };
    static std::vector<std::string> dsp_blocks(dsp_list, dsp_list + 29);

    // Control blocks
    std::string control_list[] = 
    {
        "amp_clip",
        "dummy_control",
        "energy_feedback",
        "gain_fb",
        "soft_clip",
        "speaker_size_crossover",
        "speaker_size_gain",
        "split_tone_handler",
        "tone_handler"
    };
    static std::vector<std::string> control_blocks(control_list, control_list + 9);

    static ImGuiTextFilter filter;
    filter.Draw("Search", ImGui::GetContentRegionAvail().x); // Need to fix inputTextHint in imgui.cpp
    if (ImGui::ListBoxHeader("##BlockBrowserList", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y)))
    {
        for (const auto& block_type : block_browser)
        {
            if (filter.PassFilter(block_type.c_str()))
            {
                if (ImGui::TreeNodeEx(block_type.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) 
                {
                    if (block_type == "IO Blocks") 
                    {
                        for (const auto& io_block : io_blocks) 
                        {
                            ImGui::Text("%s", io_block.c_str()); 
                            ImGui::Indent();
                            ImGui::TreePop();
                        }
                    }
                    else if (block_type == "DSP Blocks")
                    {
                        for (const auto& dsp_block : dsp_blocks) 
                        {
                            ImGui::Text("%s", dsp_block.c_str()); 
                            ImGui::Indent();
                            ImGui::TreePop();
                        }
                    }
                    else 
                    {
                        for (const auto& control_block : control_blocks) 
                        {
                            ImGui::Text("%s", control_block.c_str()); 
                            ImGui::Indent();
                            ImGui::TreePop();
                        }
                    }
                    ImGui::TreePop();
                }				
            }
        }
        ImGui::ListBoxFooter();
    }

    ImGui::End();
}

void Palette::exit() {
    // do nothing
}