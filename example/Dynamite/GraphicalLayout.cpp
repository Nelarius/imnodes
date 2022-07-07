// Initial graphical layout for Project Dynamite include menu bar and canvas view 

#include <imnodes.h>
#include <imgui.h>
#include <imgui_internal.h>
#include "dynamite_editor.h"
#include <vector>

using namespace std;

namespace Dynamite 
{

//ImNodesEditorContext* context = nullptr;

const char* App_GetName()
{
    return "Menu Bar";
}

void App_Initialize()
{
    ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);

    ImNodesIO& io = ImNodes::GetIO();
    io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
    io.MultipleSelectModifier.Modifier = &ImGui::GetIO().KeyCtrl;

    ImNodesStyle& style = ImNodes::GetStyle();
    style.Flags |= ImNodesStyleFlags_GridLinesPrimary | ImNodesStyleFlags_GridSnapping;

    //ImFontAtlas::Build

}

void App_Frame()
{
    auto flags = ImGuiWindowFlags_MenuBar;

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    static bool use_work_area = true;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
    ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);

    bool* p_open = NULL;
    ImGui::Begin("Dynamite Editor", p_open, flags);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            std::vector<char*> names { "New", "Open", "Save", "Save as", "Import", "Export", "Close"  };
            for (char* i : names) {
                if (ImGui::MenuItem(i, NULL)) {
                    printf("File menu opened!\n");
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            std::vector<char*> names { "Zoom In", "Zoom Out", "Zoom to Content" };
            for (char* i : names) {
                if (ImGui::MenuItem(i, NULL)) {
                    printf("Zoom!\n");
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Commands")) {
            std::vector<char*> names { "Validate", "Generate", "Fetch", "Deploy", "Clean" };
            for (char* i : names) {
                if (ImGui::MenuItem(i, NULL)) {
                    printf("I command you!\n");
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            std::vector<char*> names { "Help Center", "About Dynamite" };
            for (char* i : names) {
                if (ImGui::MenuItem(i, NULL)) {
                    printf("Help me!\n");
                }
            }
            ImGui::EndMenu();
        }

        const float spacing = ImGui::GetWindowWidth() - ImGui::CalcTextSize("Close Window").x - ImGui::CalcTextSize("File").x;
        ImGui::SameLine(spacing);
        if (ImGui::Button("Close window\n")) {
            printf("Closing window.\n"); 
            *p_open = false;
        }

        ImGui::EndMenuBar();
    }

    ImNodes::BeginNodeEditor();
    ImNodes::EndNodeEditor();

    ImGui::End();
}

void App_Finalize()
{
    ImNodes::PopAttributeFlag();
}

}



