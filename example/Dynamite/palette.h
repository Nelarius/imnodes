#pragma once

#include <imgui.h>
#include <vector>
#include <string>

#include "context.h"

struct Blocks
{
    std::vector<std::string> block_types, io_blocks, dsp_blocks, control_blocks;
};

// Block information used in Context
struct FromPalette
{
    bool clicked;
    std::string block_type;
    
    bool input_placed = false;
    bool output_placed = false;
};

class Palette {
    private: 
        void drawBlockBrowser(Blocks contents);
        
    public:
        bool system_clicked;
        bool block_clicked;

        Palette();
        void init();
        void show();
        void exit();
}; 