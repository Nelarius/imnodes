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
        ImGui::EndMenuBar();
    }
}

void MenuBar::exit() {
    // ImGui::EndMenuBar()
}