#pragma once

#include <imgui.h>
#include <vector>
#include <string>
#include <imnodes.h>

#include "block.h"
#include "editor.h"

#include <iostream>


struct MultiPanelFuncs {
    static int blockNameCallBack(ImGuiInputTextCallbackData* data) {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
            ((Block*)data->UserData)->setName(std::string(data->Buf));
        }
        return 0;
    }
};

class MultiPanel {

    public:
        MultiPanel();
        void init();
        void show(Editor& m_editor, Context& m_context);
        void exit();

        void showBlockInfo(Editor& m_editor, Context& m_context);
        void formatInfo(Block& block);
};