#pragma once

#include <vector>
#include <string>

class DyndspWrapper {
    public:
        std::vector<std::string> get_dsp_list();
        std::vector<std::string> get_control_list();
        std::vector<std::string> get_parameter_names(std::string block_name);
        std::vector<std::string> get_parameter_types(std::string block_name);
};