#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "imnodes.h"
#include <stdio.h>
#include <SDL.h>
#include <GL/gl3w.h>

#include <vector>

inline int make_id(int node, int attribute) { return (node << 16) | attribute; }

class SimpleNodeEditor
{
public:
    SimpleNodeEditor()
        : current_id_(0), red_nodes_(), green_nodes_(), blue_nodes_()
    {
    }

    void show()
    {
        ImGui::Begin("Simple node example");
        imnodes::BeginNodeEditor();

        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBar, IM_COL32(255, 30, 30, 255));
        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBarHovered, IM_COL32(255, 80, 80, 255));
        for (int red : red_nodes_)
        {
            const float node_width = 150.0f;
            const float node_padding = 8.0f;
            imnodes::BeginNode(red);
            imnodes::Name("Red");

            imnodes::BeginAttribute(
                make_id(red, 0), imnodes::AttributeType_Input);
            ImGui::TextUnformatted("input");
            imnodes::EndAttribute();

            ImGui::Dummy(ImVec2(node_width, 1.f));

            imnodes::BeginAttribute(
                make_id(red, 1), imnodes::AttributeType_Output);
            const auto label_size = ImGui::CalcTextSize("output");
            ImGui::Indent(node_width - label_size.x - node_padding);
            ImGui::TextUnformatted("output");
            imnodes::EndAttribute();

            imnodes::EndNode();
        }
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBarHovered);
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBar);

        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBar, IM_COL32(30, 255, 30, 255));
        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBarHovered, IM_COL32(100, 255, 100, 255));
        for (int green : green_nodes_)
        {
            const float node_width = 80.0f;
            const float node_padding = 8.0f;
            imnodes::BeginNode(green);
            imnodes::Name("Green");

            imnodes::BeginAttribute(
                make_id(green, 0), imnodes::AttributeType_Input);
            ImGui::TextUnformatted("input");
            imnodes::EndAttribute();

            {
                imnodes::BeginAttribute(
                    make_id(green, 1), imnodes::AttributeType_Internal);
                const float label_width = ImGui::CalcTextSize("color").x;
                ImGui::TextUnformatted("color");
                ImGui::SameLine();
                ImGui::PushItemWidth(node_width - label_width - node_padding);
                // TODO: figure out where to store the color values
                // ImGui::ColorEdit3("##hidelabel", );
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }

            {
                imnodes::BeginAttribute(
                    make_id(green, 1), imnodes::AttributeType_Output);
                const auto label_size = ImGui::CalcTextSize("output").x;
                ImGui::Indent(node_width - label_size - node_padding);
                ImGui::TextUnformatted("output");
                imnodes::EndAttribute();
            }

            imnodes::EndNode();
        }
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBarHovered);
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBar);

        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBar, IM_COL32(30, 30, 255, 255));
        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBarHovered, IM_COL32(80, 80, 255, 255));
        for (int blue : blue_nodes_)
        {
            const float node_width = 100.0f;
            const float node_padding = 8.0f;
            imnodes::BeginNode(blue);
            imnodes::Name("Blue");

            imnodes::BeginAttribute(
                make_id(blue, 0), imnodes::AttributeType_Input);
            ImGui::TextUnformatted("asdf");
            imnodes::EndAttribute();

            {
                imnodes::BeginAttribute(
                    make_id(blue, 1), imnodes::AttributeType_Internal);
                const float label_width = ImGui::CalcTextSize("vector").x;
                ImGui::TextUnformatted("vector");
                ImGui::SameLine();
                ImGui::PushItemWidth(node_width - label_width - node_padding);
                // TODO:
                // ImGui::DragFloat3("##hidelabel");
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }

            {
                imnodes::BeginAttribute(
                    make_id(blue, 2), imnodes::AttributeType_Output);
                const float label_width = ImGui::CalcTextSize("asdfasdf").x;
                ImGui::Indent(node_width - label_width);
                ImGui::TextUnformatted("asdfasdf");
                imnodes::EndAttribute();
            }

            imnodes::EndNode();
        }
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBarHovered);
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBar);

        // Context menu for adding new nodes

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));

        if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseHoveringWindow() &&
            ImGui::IsMouseClicked(1))
        {
            ImGui::OpenPopup("context menu");
        }

        if (ImGui::BeginPopup("context menu"))
        {
            int new_node = -1;
            ImVec2 click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();

            if (ImGui::MenuItem("Red"))
            {
                new_node = current_id_++;
                red_nodes_.push_back(new_node);
            }

            if (ImGui::MenuItem("Blue"))
            {
                new_node = current_id_++;
                blue_nodes_.push_back(new_node);
            }

            if (ImGui::MenuItem("Green"))
            {
                new_node = current_id_++;
                green_nodes_.push_back(new_node);
            }

            ImGui::EndPopup();

            if (new_node != -1)
            {
                imnodes::SetNodePos(new_node, click_pos, ImGuiCond_Appearing);
            }
        }
        ImGui::PopStyleVar();
        imnodes::EndNodeEditor();

        ImGui::End();
    }

private:
    int current_id_;
    std::vector<int> red_nodes_;
    std::vector<int> green_nodes_;
    std::vector<int> blue_nodes_;
};

int main(int, char**)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

#if __APPLE__
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_FLAGS,
        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    SDL_Window* window = SDL_CreateWindow(
        "imgui-node-editor example",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    if (gl3wInit())
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    imnodes::Initialize();

    // Setup style
    ImGui::StyleColorsDark();

    // Simple node editor
    SimpleNodeEditor node_example;

    bool done = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        node_example.show();

        // Rendering
        ImGui::Render();
        SDL_GL_MakeCurrent(window, gl_context);
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(
            clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    imnodes::Shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}