#pragma once

#include <vector>
#include <string>

#include "context.h"

class DyndspWrapper {

    std::string *ip_address;

    public:

        // Grabs ip address from the context before deploying system onto player
        void getData(Context &m_context);

        // Generic dyndsp caller for all player commands
        void call_dyndsp_command(std::string command); 
        
        static std::vector<std::string> get_dsp_list();
        static std::vector<std::string> get_control_list();
        static std::vector<std::string> get_parameter_names(std::string block_name);
        static std::vector<std::string> get_parameter_types(std::string block_name);
};