#include <imgui.h>
#include <imnodes.h>

#include "ui.h"
// #include "state.h"
#include <string>

using namespace std;

class DSPDynamite {

public: 

    UI ui;
    //const string name;
    ImNodesEditorContext* context; 

    DSPDynamite();
    void init();
    void show(bool done);
    void exit();
};