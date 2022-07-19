#pragma once

#include <imgui.h>
#include <vector>
#include <string>

struct Blocks
{
    std::vector<std::string> block_types, io_blocks, dsp_blocks, control_blocks;
};

class Palette {
    private: 
        std::vector<std::string> listDSPBlocks();
        std::vector<std::string> listControlBlocks();
        void drawBlockBrowser(Blocks contents);
    public:
        Palette();
        void init();
        void show(float s_LeftPaneSize);
        void exit();
}; 