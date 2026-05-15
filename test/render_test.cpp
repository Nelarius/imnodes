// Headless render test for imnodes.
//
// Renders a small node graph to an offscreen framebuffer and saves it as a PNG.
// Intended for agents to iterate on imnodes changes and inspect the visual
// result directly, without needing an interactive window.
//
// Usage: render_test [output.png]
// Default output path: imnodes_test.png in the current working directory.
//
// Implementation notes:
// - SDL2 + OpenGL 3.2 Core (matches the existing examples in example/main.cpp)
//   so that the imgui_impl_opengl3 backend can be reused unchanged.
// - The SDL window is created hidden. We render into a user FBO and call
//   glReadPixels to recover RGBA pixels. The window is only needed to obtain
//   a valid GL context; the default framebuffer is never presented.
// - ImGui needs an explicit display size and a non-zero delta-time before its
//   first NewFrame, since we are not driving it from the SDL event loop. We
//   set DisplaySize from the target image size and fix DeltaTime to 1/60.
// - We pump several frames before reading pixels. imnodes lazily initializes
//   editor / node sizes on the first layout pass, and links route between
//   pin positions that are not known until after at least one node has been
//   submitted. Two frames are enough in practice; we use four to be safe.

#include <imnodes.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <SDL2/SDL.h>
// FBOs / glReadPixels into a user texture require the GL 3.x core API. The
// platform headers used by the existing examples (<SDL2/SDL_opengl.h>) pull in
// the legacy desktop GL headers, which on macOS do not expose FBO entry points
// at all. Include the core-profile headers directly here.
#if defined(__APPLE__)
#  define GL_SILENCE_DEPRECATION
#  include <OpenGL/gl3.h>
#elif defined(_WIN32)
#  include <SDL2/SDL_opengl.h>
#else
#  define GL_GLEXT_PROTOTYPES
#  include <GL/gl.h>
#  include <GL/glext.h>
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace
{

// Target image dimensions. Kept modest so PNGs are quick to write and easy to
// eyeball in a terminal image viewer.
constexpr int kImageWidth  = 1024;
constexpr int kImageHeight = 640;

// Number of frames to pump before reading pixels. See file header comment.
constexpr int kWarmupFrames = 4;

// Build a small node graph: two source nodes feeding one sink. This exercises
// node title bars, input/output pins, and link rendering between them.
void build_node_graph()
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)kImageWidth, (float)kImageHeight));
    ImGui::Begin(
        "render_test",
        nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImNodes::BeginNodeEditor();

    // Node 1: source A
    ImNodes::BeginNode(1);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted("source A");
    ImNodes::EndNodeTitleBar();
    ImNodes::BeginOutputAttribute(11);
    ImGui::Indent(60);
    ImGui::Text("out");
    ImNodes::EndOutputAttribute();
    ImNodes::EndNode();

    // Node 2: source B
    ImNodes::BeginNode(2);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted("source B");
    ImNodes::EndNodeTitleBar();
    ImNodes::BeginOutputAttribute(21);
    ImGui::Indent(60);
    ImGui::Text("out");
    ImNodes::EndOutputAttribute();
    ImNodes::EndNode();

    // Node 3: sink (two inputs)
    ImNodes::BeginNode(3);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted("sink");
    ImNodes::EndNodeTitleBar();
    ImNodes::BeginInputAttribute(31);
    ImGui::Text("a");
    ImNodes::EndInputAttribute();
    ImNodes::BeginInputAttribute(32);
    ImGui::Text("b");
    ImNodes::EndInputAttribute();
    ImNodes::BeginOutputAttribute(33);
    ImGui::Indent(60);
    ImGui::Text("result");
    ImNodes::EndOutputAttribute();
    ImNodes::EndNode();

    // Links: A.out -> sink.a, B.out -> sink.b
    ImNodes::Link(100, 11, 31);
    ImNodes::Link(101, 21, 32);

    ImNodes::EndNodeEditor();
    ImGui::End();
}

// Flip image rows in-place. OpenGL returns pixels bottom-up; PNG expects top-
// down. Operates on 4-byte (RGBA) pixels.
void flip_rows_rgba(unsigned char* pixels, int width, int height)
{
    const int row_bytes = width * 4;
    std::vector<unsigned char> tmp(row_bytes);
    for (int y = 0; y < height / 2; ++y)
    {
        unsigned char* a = pixels + y * row_bytes;
        unsigned char* b = pixels + (height - 1 - y) * row_bytes;
        std::copy(a, a + row_bytes, tmp.begin());
        std::copy(b, b + row_bytes, a);
        std::copy(tmp.begin(), tmp.end(), b);
    }
}

} // namespace

int main(int argc, char** argv)
{
    const char* out_path = (argc > 1) ? argv[1] : "imnodes_test.png";

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // GL 3.2 Core, matching example/main.cpp so imgui_impl_opengl3 is happy.
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window* window = SDL_CreateWindow(
        "imnodes_render_test",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        kImageWidth,
        kImageHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!window)
    {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_ctx = SDL_GL_CreateContext(window);
    if (!gl_ctx)
    {
        std::fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_MakeCurrent(window, gl_ctx);

    // Offscreen render target. We render into this FBO and read back from it,
    // so the visible state of the (hidden) window is irrelevant.
    GLuint fbo = 0, color_tex = 0;
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &color_tex);
    glBindTexture(GL_TEXTURE_2D, color_tex);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        kImageWidth,
        kImageHeight,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::fprintf(stderr, "FBO incomplete\n");
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImNodes::CreateContext();
    ImGui::StyleColorsDark();
    ImNodes::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize    = ImVec2((float)kImageWidth, (float)kImageHeight);
    io.DeltaTime      = 1.0f / 60.0f;
    // The SDL2 backend would normally set this from the window event loop;
    // since we are not pumping events we disable any feature that depends on
    // dynamic mouse capture / cursor management.
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    ImGui_ImplSDL2_InitForOpenGL(window, gl_ctx);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Position the nodes in editor (grid) space. This must happen after the
    // imnodes context is created. Coordinates are in editor space, which is
    // independent of the editor window's panning state at frame zero.
    ImNodes::SetNodeGridSpacePos(1, ImVec2(60.f, 100.f));
    ImNodes::SetNodeGridSpacePos(2, ImVec2(60.f, 260.f));
    ImNodes::SetNodeGridSpacePos(3, ImVec2(480.f, 180.f));

    for (int frame = 0; frame < kWarmupFrames; ++frame)
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        build_node_graph();

        ImGui::Render();

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, kImageWidth, kImageHeight);
        // Match the clear color of the existing examples (a muted slate) so
        // visual diffs against screenshots of the interactive examples are
        // meaningful.
        glClearColor(0.45f * 1.0f, 0.55f * 1.0f, 0.60f * 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    // Read back the final frame.
    std::vector<unsigned char> pixels(kImageWidth * kImageHeight * 4);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(
        0, 0, kImageWidth, kImageHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    flip_rows_rgba(pixels.data(), kImageWidth, kImageHeight);

    if (!stbi_write_png(
            out_path, kImageWidth, kImageHeight, 4, pixels.data(), kImageWidth * 4))
    {
        std::fprintf(stderr, "stbi_write_png failed for '%s'\n", out_path);
        return 1;
    }
    std::printf("wrote %s (%dx%d)\n", out_path, kImageWidth, kImageHeight);

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImNodes::DestroyContext();
    ImGui::DestroyContext();

    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &color_tex);
    SDL_GL_DeleteContext(gl_ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
