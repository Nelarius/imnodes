#pragma once

#include "multipanel.h"
#include <imgui.h>
#include <vector>
#include <string>

class MultiPanel {
    public:
        MultiPanel();
        void init();
        void show();
        void exit();
};