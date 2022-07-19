#include <imgui.h>
#include "dynamite.h"

int main (int, char**) 
{
    Dynamite dynamite; 
    dynamite.python_test();
    dynamite.init();

    bool done = false;
    while (!done) {
        done = dynamite.show(done);
    }
    
    dynamite.exit();
}