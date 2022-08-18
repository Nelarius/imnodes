#pragma once

#include "ui.h"
#include "context.h"
#include "dyndsp_wrapper.h"

using namespace std;

class Dynamite {

public: 

    UI m_ui; 
    Context m_context;
    DyndspWrapper m_wrapper;

    Dynamite();
    void init();
    bool show(bool done);
    void exit();
};