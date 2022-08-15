#pragma once

#include <vector>
#include <string>
#include <fstream>

#include <Python.h>
#include "pyhelper.h"

using namespace std;

class DyndspWrapper {
    public:
        static void generic_wrapper(string command);
        static void validate();
        static void generate();
        static void deploy();
        static std::vector<std::string> get_dsp_list();
        static std::vector<std::string> get_control_list();
        static std::vector<std::string> get_parameter_names(std::string block_name);
        static std::vector<std::string> get_parameter_types(std::string block_name);
};