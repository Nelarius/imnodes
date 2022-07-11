#include "menubar.h"
#include <string>

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
            std::vector<std::string> MenuItems { "New", "Open", "Save", "Save as", "Import", "Export", "Close"  };
            for (auto Item : MenuItems) {
                if (ImGui::MenuItem(Item.c_str(), NULL)) {
                    printf("File menu opened!\n");
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            std::vector<std::string> menuItems { "Zoom In", "Zoom Out", "Zoom to Content" };
            for (auto item : menuItems) {
                if (ImGui::MenuItem(item.c_str(), NULL)) {
                    printf("Zoom!\n");
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Commands")) {
            std::vector<std::string> menuItems { "Validate", "Generate", "Fetch", "Deploy", "Clean" };
            for (auto item : menuItems) {
                if (ImGui::MenuItem(item.c_str(), NULL)) {
                    printf("I command you!\n");
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            std::vector<std::string> menuItems { "Help Center", "About Dynamite" };
            for (auto item : menuItems) {
                if (ImGui::MenuItem(item.c_str(), NULL)) {
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