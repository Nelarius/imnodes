#pragma once

#include <imgui.h>
#include <imnodes.h>

#include "ui.h"
// #include "state.h"
#include <string>

using namespace std;

class Dynamite {

public: 

    UI m_ui; 
    ImNodesEditorContext* m_context; 
    //Context m_context;

    Dynamite();
    void init();
    int python_test();
    bool show(bool done);
    void exit();
};