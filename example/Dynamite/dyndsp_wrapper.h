#pragma once

#include <vector>
#include <string>

#include "context.h"

using namespace std;

struct DynDSPData {
    std::string ip_address;
    // sys_name
    // chirp_en
    // trueplay_en
};

class DyndspWrapper {

    std::string *ip_address;
    std::string sys_name;
    bool chirp_en;
    bool trueplay_en;

    public:

        void getData(Context &m_context);
        void generic_wrapper(std::string command);
        static void deploy();
        static std::vector<std::string> get_dsp_list();
        static std::vector<std::string> get_control_list();
        static std::vector<std::string> get_parameter_names(std::string block_name);
        static std::vector<std::string> get_parameter_types(std::string block_name);
};