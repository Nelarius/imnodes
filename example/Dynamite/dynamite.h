#pragma once

#include <imgui.h>
#include <imnodes.h>

#include "ui.h"
#include "context.h"
#include "JsonGraphFileWriter.h"
#include <string>

using namespace std;

class Dynamite {

public: 

    UI m_ui; 
    //ImNodesEditorContext* context; 
    Context m_context;

    Dynamite();
    void init();
    bool show(bool done);
    void exit();
};