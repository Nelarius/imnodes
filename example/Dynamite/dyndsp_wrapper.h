#pragma once

#include <vector>
#include <string>

class DyndspWrapper {
    public:
        std::vector<std::string> get_dsp_list();
        std::vector<std::string> get_control_list();
};