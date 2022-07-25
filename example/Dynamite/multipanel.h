#pragma once

#include <imgui.h>
#include <vector>
#include <string>
#include <imnodes.h>

#include "block.h"
#include "editor.h"

class MultiPanel {

    public:
        MultiPanel();
        void init();
        void show(Editor m_editor, Context m_context);
        void exit();

        void showBlockInfo(Editor m_editor, Context m_context);
        void formatInfo(Block block);
};