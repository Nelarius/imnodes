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

bool UI::show(bool done, Context &m_context, DyndspWrapper m_wrapper) {
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

    // Set up base app window
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), 0, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y * 0.75f));

    // Render base app window
    auto flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;
    ImGui::Begin("Dynamite Editor", NULL, flags);

    // Render the menu bar
    m_menu.show(m_context, m_wrapper); 

    /* This is the hacky way that Jehan and Kevin
    have done to manage windows. This part of the code
    should be fixed when the "docking" branch is 
    pulled to this repo.*/
    // Splitter between palette and editor
    UI::setSplitter();
    Splitter(true, s_SplitterSize, &s_LeftPaneSize, &s_RightPaneSize, 100.0f, 100.0f);

    // Render 
    ImGui::BeginChild("##palette", ImVec2(s_LeftPaneSize, -1), false, 0);
    m_palette.show();
    ImGui::EndChild();
    ImGui::SameLine(0.0f, s_SplitterSize);

    // Render the editor
    ImGui::BeginChild("##central canvas", ImVec2(s_RightPaneSize, -1), false, 0);
    m_editor.show(m_context); 
    ImGui::EndChild(); 

    m_context.m_graph.addLink();
    int link_id = 0;
    m_context.m_graph.deleteLink(link_id);

    /* This is the hacky way that Jehan and Kevin
    have done to manage windows. This part of the code
    should be fixed when the "docking" branch is 
    pulled to this repo.*/
    // Set (hard code) multi-purpose position and size
    ImGui::SetNextWindowPos(ImVec2(0.0f, ImGui::GetIO().DisplaySize.y * 0.755f));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y * 0.246f));

    // Render multi-purpose panel
    flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    ImGui::Begin("##multipanel", NULL, flags);
    m_multipanel.show(m_editor, m_palette, m_context); 
    ImGui::End();

    // End menu bar + canvas + multipanel window
    ImGui::End();

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
    if (s_SplitterArea != availableRegion.x) 
    {
        if (s_SplitterArea == 0.0f) {
            s_SplitterArea     = availableRegion.x;
            s_LeftPaneSize     = ImFloor(availableRegion.x * 0.2f);
            s_RightPaneSize    = availableRegion.x - s_LeftPaneSize - s_SplitterSize;
        } else {
            auto ratio = availableRegion.x / s_SplitterArea;
            s_SplitterArea     = availableRegion.x;
            s_LeftPaneSize     = s_LeftPaneSize * ratio;
            s_RightPaneSize    = availableRegion.x - s_LeftPaneSize - s_SplitterSize;
        }
    }
} 
