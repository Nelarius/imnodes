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
        void drawBlockBrowser(Blocks contents);
    public:
        Palette();
        void init();
        void show();
        void exit();
}; 