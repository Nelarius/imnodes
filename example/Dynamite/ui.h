#pragma once

#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include <imnodes.h>

#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
#include <SDL2/SDL.h>
#include <SDL_scancode.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL2/SDL_opengles2.h>
#else
#include <SDL2/SDL_opengl.h>
#endif

#include <iostream>
#include <vector>
#include <string>

#include "menubar.h"
#include "editor.h" 
#include "palette.h" 
#include "multipanel.h" 
#include "context.h"
#include "dyndsp_wrapper.h"

class UI {

public:

    float s_SplitterSize    = 6.0f;
    float s_SplitterArea    = 0.0f;
    float s_LeftPaneSize    = 0.0f;
    float s_RightPaneSize   = 0.0f; 
    float s_TopPaneSize    = 0.0f;
    float s_BottomPaneSize   = 0.0f; 

    MenuBar m_menu;
    Editor m_editor;
    Palette m_palette;
    MultiPanel m_multipanel;

    SDL_Window* m_window;
    SDL_GLContext gl_context;
    ImGuiIO m_io;
    ImVec4 m_clear_color;

    static bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f) {
        using namespace ImGui;
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiID id = window->GetID("##Splitter");
        ImRect bb;
        bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
        bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
        return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
    }

    UI();
    void init();
    bool show(bool done, Context &m_context, DyndspWrapper m_wrapper);
    void exit();
    void setSplitter();

};