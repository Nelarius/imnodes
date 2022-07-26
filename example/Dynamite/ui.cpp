#include "ui.h"

UI::UI() { 
    // from main
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        //return -1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_MAXIMIZED);
    m_window = SDL_CreateWindow(
        "Dynamite",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        window_flags);

    gl_context = SDL_GL_CreateContext(m_window);
    SDL_GL_MakeCurrent(m_window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    m_io = ImGui::GetIO();
    (void)m_io;

    ImNodes::CreateContext();
    ImGui::StyleColorsDark();
    ImNodes::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(m_window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    m_clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
}

void UI::init() {
    m_editor.init();
    m_palette.init();
}

bool UI::show(bool done, Context &m_context) {
    (void) done;
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) 
            return done = true;

        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(m_window))
            return done = true;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // set up UI
    ImGui::SetNextWindowPos(ImVec2(0.1f, 0.0f), 0, ImVec2(-0.25f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x * 0.8f, ImGui::GetIO().DisplaySize.y));
    auto flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;
    ImGui::Begin("Dynamite Editor", NULL, flags); // begin menu bar + canvas + multipanel window
    m_menu.show(); 

    UI::setSplitter();
    Splitter(false, s_SplitterSize, &s_TopPaneSize, &s_BottomPaneSize, 100.0f, 100.0f);
    ImGui::BeginChild("##central canvas", ImVec2(-1, s_TopPaneSize), false, 0);
    m_editor.show(m_context);
    ImGui::EndChild();
    
    ImGui::BeginChild("##multipanel", ImVec2(-1, s_BottomPaneSize), false, 0);
    m_multipanel.show(m_editor, m_context); 
    ImGui::EndChild(); 

    m_context.addLink();
    int link_id = 0;
    m_context.deleteLink(link_id);

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x * 0.2f, ImGui::GetIO().DisplaySize.y));
    flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    ImGui::Begin("##palette", NULL, flags);
    m_palette.show(); 
    ImGui::End();

    ImGui::End(); // end menu bar + canvas + multipanel window

    /*
    auto availableRegion = ImGui::GetContentRegionAvail();
    static float s_SplitterSize     = 6.0f;
    static float s_SplitterArea     = 0.0f;
    static float s_TopPaneSize     = 0.0f;
    static float s_BottomPaneSize    = 0.0f;

    if (s_SplitterArea != availableRegion.y) {
        if (s_SplitterArea == 0.0f) {
            s_SplitterArea     = availableRegion.y;
            s_TopPaneSize     = ImFloor(availableRegion.y * 0.75f);
            s_BottomPaneSize    = availableRegion.y - s_TopPaneSize - s_SplitterSize;
        } else {
            auto ratio = availableRegion.y / s_SplitterArea;
            s_SplitterArea     = availableRegion.y;
            s_TopPaneSize     = s_TopPaneSize * ratio;
            s_BottomPaneSize    = availableRegion.y - s_TopPaneSize - s_SplitterSize;
        }
    } 
    Splitter(false, s_SplitterSize, &s_TopPaneSize, &s_BottomPaneSize, 100.0f, 100.0f); 
    ImGui::BeginChild("##palette", ImVec2(-1, s_TopPaneSize), false, ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::EndChild();
    ImGui::SameLine(0.0f, s_SplitterSize);
    ImGui::BeginChild("##canvas", ImVec2(-1, s_BottomPaneSize), false, ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::EndChild(); */
    /*
    auto availableRegion = ImGui::GetContentRegionAvail();
    printf("available region.x = %f \n", availableRegion.x);
    static float s_SplitterSize     = 6.0f;
    static float s_SplitterArea     = 0.0f;
    static float s_LeftPaneSize     = 0.0f;
    static float s_RightPaneSize    = 0.0f;

    if (s_SplitterArea != availableRegion.x) {
        if (s_SplitterArea == 0.0f) {
            s_SplitterArea     = availableRegion.x;
            s_LeftPaneSize     = ImFloor(availableRegion.x * 0.33f);
            s_RightPaneSize    = availableRegion.x - s_LeftPaneSize - s_SplitterSize;
        } else {
            auto ratio = availableRegion.x / s_SplitterArea;
            s_SplitterArea     = availableRegion.x;
            s_LeftPaneSize     = s_LeftPaneSize * ratio;
            s_RightPaneSize    = availableRegion.x - s_LeftPaneSize - s_SplitterSize;
        }
    }

    Splitter(true, s_SplitterSize, &s_LeftPaneSize, &s_RightPaneSize, 50.0f, 50.0f);
    ImGui::BeginChild("##palette", ImVec2(s_LeftPaneSize, 0.0), false, 0); 
    ImGui::EndChild();
    ImGui::SameLine(0.0f, s_SplitterSize);

    ImGui::BeginChild("##right hand side", ImVec2(s_RightPaneSize, 0.0), false, 0); 
    availableRegion = ImGui::GetContentRegionAvail();
    s_SplitterArea     = 0.0f;
    s_LeftPaneSize     = 0.0f;
    s_RightPaneSize    = 0.0f;

    if (s_SplitterArea != availableRegion.x) {
        if (s_SplitterArea == 0.0f) {
            s_SplitterArea     = availableRegion.x;
            s_LeftPaneSize     = ImFloor(availableRegion.x * 0.33f);
            s_RightPaneSize    = availableRegion.x - s_LeftPaneSize - s_SplitterSize;
        } else {
            auto ratio = availableRegion.x / s_SplitterArea;
            s_SplitterArea     = availableRegion.x;
            s_LeftPaneSize     = s_LeftPaneSize * ratio;
            s_RightPaneSize    = availableRegion.x - s_LeftPaneSize - s_SplitterSize;
        }
    }

    Splitter(true, s_SplitterSize, &s_LeftPaneSize, &s_RightPaneSize, 50.0f, 50.0f);
    ImGui::BeginChild("##editor", ImVec2(s_LeftPaneSize, 0.0), false, 0); 
    m_editor.show(m_context);
    ImGui::EndChild();
    ImGui::SameLine(0.0f, s_SplitterSize);

    ImGui::BeginChild("##multipurpose panel", ImVec2(0.0, 0.0), false, 0); 
    ImGui::EndChild();

    ImGui::EndChild(); */

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)m_io.DisplaySize.x, (int)m_io.DisplaySize.y);
    glClearColor(
        m_clear_color.x * m_clear_color.w,
        m_clear_color.y * m_clear_color.w,
        m_clear_color.z * m_clear_color.w,
        m_clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(m_window);

    return done;
}

void UI::exit() {
    m_editor.exit();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    
    ImNodes::DestroyContext();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void UI::setSplitter() {
    auto availableRegion = ImGui::GetContentRegionAvail();
    if (s_SplitterArea != availableRegion.y)
    {
        if (s_SplitterArea == 0.0f)
        {
            s_SplitterArea     = availableRegion.y;
            s_TopPaneSize     = ImFloor(availableRegion.y * 0.75f);
            s_BottomPaneSize    = availableRegion.y - s_TopPaneSize - s_SplitterSize;
        }
        else
        {
            auto ratio = availableRegion.y / s_SplitterArea;
            s_SplitterArea     = availableRegion.y;
            s_TopPaneSize     = s_TopPaneSize * ratio;
            s_BottomPaneSize    = availableRegion.y - s_TopPaneSize - s_SplitterSize;
        }
    }
} 
