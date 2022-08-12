#pragma once

#include <imgui.h>
#include <vector>
#include <string>
#include <imnodes.h>

#include "block.h"
#include "editor.h"
#include "palette.h"

struct MultiPanelFuncs {
    static int blockNameCallBack(ImGuiInputTextCallbackData* data) {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
            ((Block*)data->UserData)->setName(std::string(data->Buf));
        }
        return 0;
    }

    static int systemNameCallBack(ImGuiInputTextCallbackData* data) {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
            ((Context*)data->UserData)->system_name = (std::string(data->Buf));
        }
        return 0;
    }

    static int ipAddressCallBack(ImGuiInputTextCallbackData* data) {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
            ((Context*)data->UserData)->target_ip_address = (std::string(data->Buf));
        }
        return 0;
    }
};

class MultiPanel {

    public:
        MultiPanel();
        void init();
        void show(Editor& m_editor, Palette& m_palette, Context& m_context);
        void exit();

        void showSystemInfo(Context& m_context);
        void showBlockInfo(Editor& m_editor, Context& m_context);
        void formatInfo(Block& block);
};