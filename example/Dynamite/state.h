#pragma once

#include "dyndsp_wrapper.h"

struct BlockNames 
{
    std::vector<std::string> dsp_names, control_names;
};

class State {
    public:
        DyndspWrapper m_wrapper;

        void init();
};