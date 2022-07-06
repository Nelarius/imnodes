// Initial graphical layout for Project Dynamite include menu bar and canvas view 

#include <imnodes.h>
#include <imgui.h>
#include <imgui_internal.h>
#include "dynamite_editor.h"

namespace Dynamite 
{

//ImNodesEditorContext* context = nullptr;

const char* App_GetName()
{
    return "Menu Bar";
}

void App_Initialize()
{
    //context = ImNodes::EditorContextCreate(); 
    ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);

    ImNodesIO& io = ImNodes::GetIO();
    io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
    io.MultipleSelectModifier.Modifier = &ImGui::GetIO().KeyCtrl;

    ImNodesStyle& style = ImNodes::GetStyle();
    style.Flags |= ImNodesStyleFlags_GridLinesPrimary | ImNodesStyleFlags_GridSnapping;

}

void App_Frame()
{
    ImGui::ShowDemoWindow(NULL);
    auto flags = ImGuiWindowFlags_MenuBar;

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    static bool use_work_area = true;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
    ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);

    //ImNodes::EditorContextSet(context);
    bool* p_open = NULL;
    //*p_open = true;
    ImGui::Begin("Dynamite Editor", p_open, flags);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            const char* names[] = {
                "New",
                "Open",
                "Save",
                "Save as",
                "Import",
                "Export",
                "Close",
            };

            for (int i = 0; i < 7; i++)
            {
                if (ImGui::MenuItem(names[i], NULL))
                    printf("Menu item clicked!");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            const char* names[] = {
                "Zoom in",
                "Zoom out",
                "Zoom to content",
            };

            for (int i = 0; i < 3; i++)
            {
                if (ImGui::MenuItem(names[i], NULL))
                    printf("Zoom!");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Commands"))
        {
            const char* names[] = {
                "Validate",
                "Generate",
                "Fetch",
                "Deploy",
                "Clean",
            };

            for (int i = 0; i < 5; i++)
            {
                if (ImGui::MenuItem(names[i], NULL))
                    printf("I command you!");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            const char* names[] = {
                "Help Center",
                "About Dynamite",
            };

            for (int i = 0; i < 2; i++)
            {
                if (ImGui::MenuItem(names[i], NULL))
                    printf("Help me!");
            }
            ImGui::EndMenu();
        }

        ImGui::SameLine(ImGui::GetWindowWidth() - 140); // ImGui::GetWindowWidth() - sizeof(ImGui::Button("Close this window"))
        if (ImGui::Button("Close window"))
        {
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
    //ImNodes::EditorContextFree(context);
}

}



