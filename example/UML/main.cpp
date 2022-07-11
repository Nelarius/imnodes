#include <imgui.h>
#include "dynamite.h"
#include "ui.h"
#include "menubar.h"

DSPDynamite dynamite;

int main (int, char**) 
{
    dynamite.init();

    bool done = false;
    while (!done) {
        dynamite.show(done);
    }

    dynamite.exit();
}