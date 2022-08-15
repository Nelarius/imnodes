#pragma once

#include <vector>
#include <string>
#include <fstream>

#include <Python.h>
#include "pyhelper.h"

class DyndspWrapper {
    public:
        void validate();
        std::vector<std::string> get_dsp_list();
        std::vector<std::string> get_control_list();
        std::vector<std::string> get_parameter_names(std::string block_name);
        std::vector<std::string> get_parameter_types(std::string block_name);
};