#include "menubar.h"

using namespace std;

MenuBar::MenuBar() {
    // do nothing
}

void MenuBar::init() {
    // ImGui::BeginMenuBar()
}

void MenuBar::show() {
    auto flags = ImGuiWindowFlags_MenuBar;
    
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

    ImGui::End();
}

void MenuBar::exit() {
    // ImGui::EndMenuBar()
}