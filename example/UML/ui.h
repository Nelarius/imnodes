#include <imgui.h>
#include <imnodes.h>

#include "menubar.cpp" // include header files not cpp files 
#include "canvaseditor.cpp" // .h 

#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
#include <imnodes.h>
#include <SDL2/SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL2/SDL_opengles2.h>
#else
#include <SDL2/SDL_opengl.h>
#endif

#include <vector>

class UI {

public:

    MenuBar menu;
    CanvasEditor canvas;
    // Palette palette;

    SDL_Window* window;
    SDL_GLContext gl_context;
    ImGuiIO io;
    ImVec4 clear_color;

    UI();
    void init();
    void show(ImNodesEditorContext* context, bool done);
    void exit(ImNodesEditorContext* context);

};