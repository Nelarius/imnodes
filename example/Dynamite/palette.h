#pragma once

#include <imgui.h>
#include <vector>
#include <string>

#include "context.h"

struct Blocks
{
    std::vector<std::string> block_types, io_blocks, dsp_blocks, control_blocks;
};

struct FromPalette
{
    bool clicked;
    std::string block_name;
};

struct PaletteFuncs {
    static int systemNameCallBack(ImGuiInputTextCallbackData* data) {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
            ((Context*)data->UserData)->system_name = (std::string(data->Buf));
        }
        return 0;
    }
};

class Palette {
    private: 
        void drawBlockBrowser(Blocks contents);
        void drawSystemInfo(Context &m_context);
    public:
        Palette();
        void init();
        void show(Context &m_context);
        void exit();
}; 