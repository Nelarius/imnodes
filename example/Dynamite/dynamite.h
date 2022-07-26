#pragma once

#include <imgui.h>
#include <imnodes.h>

#include "ui.h"
#include "context.h"
#include "state.h"
#include <string>

using namespace std;

class Dynamite {

public: 

    UI m_ui; 
    //ImNodesEditorContext* context; 
    Context m_context;

    State m_state;

    Dynamite();
    void init();
    bool show(bool done);
    void exit();
};