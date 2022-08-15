#include "menubar.h"

static bool save = false;
static bool command = false;
static void saveToJson(Context& m_context);

CPyInstance hInst;

void MenuBar::show(Context& m_context) {
    if (save) saveToJson(m_context);
    if (command)  {
        m_context.m_wrapper.validate();
        command = false;
    }

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            vector<std::string> menuItems { "New", "Open", "Save", "Save as", "Import", "Export", "Close"  };
            //createMenu(menuItems);
            for (auto Item : menuItems) {
                if (strcmp(Item.c_str(), "Save") == 0) {
                    ImGui::MenuItem(Item.c_str(), NULL, &save);
                } else if (ImGui::MenuItem(Item.c_str(), NULL)) {
                    printf("menu opened!\n");
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            vector<std::string> menuItems { "Zoom In", "Zoom Out", "Zoom to Content" };
            //createMenu(menuItems);
            for (auto Item : menuItems) {
                if (ImGui::MenuItem(Item.c_str(), NULL)) {
                    printf("menu opened!\n");
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Commands")) {
            vector<std::string> menuItems { "Validate", "Generate", "Fetch", "Deploy", "Clean" };
            //createMenu(menuItems);
            for (auto Item : menuItems) {
                ImGui::MenuItem(Item.c_str(), NULL, &command);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            vector<std::string> menuItems { "Help Center", "About Dynamite" };
            //createMenu(menuItems);
            for (auto Item : menuItems) {
                if (ImGui::MenuItem(Item.c_str(), NULL)) {
                    printf("menu opened!\n");
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    } 
}

static void saveToJson(Context& m_context) {
    JsonGraphFileWriter fw;
    fw.writeToFile(m_context);
    save = false;
}

/*
void createMenu(vector<std::string> menuItems) {
    for (auto Item : menuItems) {
        if (ImGui::MenuItem(Item.c_str(), NULL)) {
            printf("menu opened!\n");
        }
    }
}
*/