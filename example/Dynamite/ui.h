#pragma once

#include <imgui.h>
#include <imnodes.h>

#include "menubar.h" // include header files not cpp files 
#include "editor.h" // .h 
#include "palette.h" // .h

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

    MenuBar m_menu;
    Editor m_editor;
    Palette m_palette;

    SDL_Window* m_window;
    SDL_GLContext gl_context;
    ImGuiIO m_io;
    ImVec4 m_clear_color;

    UI();
    void init();
    bool show(bool done);
    void exit(ImNodesEditorContext* context);

};