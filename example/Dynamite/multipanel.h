#pragma once

#include "multipanel.h"
#include <imgui.h>
#include <vector>
#include <string>
#include <imnodes.h>

class MultiPanel {
    public:
        MultiPanel();
        void init();
        void show();
        void displayBlockInfo();
        void exit();
};