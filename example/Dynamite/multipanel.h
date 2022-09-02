#pragma once

#include <imgui.h>
#include <vector>
#include <string>
#include <imnodes.h>

#include "block.h"
#include "editor.h"
#include "palette.h"

struct MultiPanelFuncs {
    // Callback function to save the inputted block name
    static int blockNameCallBack(ImGuiInputTextCallbackData* data) {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
            ((Block*)data->UserData)->setName(std::string(data->Buf));
        }
        return 0;
    }

    // Callback function to save the inputted system name
    static int systemNameCallBack(ImGuiInputTextCallbackData* data) {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
            ((Context*)data->UserData)->system_name = (std::string(data->Buf));
        }
        return 0;
    }

    // Callback function to save the inputted ip address
    static int ipAddressCallBack(ImGuiInputTextCallbackData* data) {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
            ((Context*)data->UserData)->target_ip_address = (std::string(data->Buf));
        }
        return 0;
    }
};

class MultiPanel {

    public:
        MultiPanel() = default;

        // Doesnt't need to do anything
        void init();

        //Display the multipurpose panel 
        void show(Editor& m_editor, Palette& m_palette, Context& m_context);

        // Doesn't need to do anything
        void exit();

        // Renders the "System Configuration" tab
        void showSystemInfo(Context& m_context);

        // Displays information for block that is clicked in the editor
        void showBlockInfo(Editor& m_editor, Context& m_context);

        // Helper for showBlockInfo
        void formatInfo(Block& block);
};