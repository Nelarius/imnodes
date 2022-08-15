#include "menubar.h"

// Forward declarations
static void saveToJson(Context& m_context);

static bool save = false;
static bool validate = false;
static bool generate_bin = false;
static bool deploy = false;
static bool clean = false;

CPyInstance hInst;

void MenuBar::show(Context& m_context) {
    if (save) saveToJson(m_context);
    if (validate) {
        m_context.m_wrapper.generic_wrapper(GET_VAR_NAME(validate));
        validate = false;
    }
    if (generate_bin) {
        m_context.m_wrapper.generic_wrapper(GET_VAR_NAME(generate_bin));
        generate_bin = false;
    }
    if (deploy) {
        m_context.m_wrapper.generic_wrapper(GET_VAR_NAME(deploy));
        deploy = false;
    }
    if (clean) {
        m_context.m_wrapper.generic_wrapper(GET_VAR_NAME(clean));
        clean = false;
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
            ImGui::MenuItem("Validate", NULL, &validate);
            ImGui::MenuItem("Generate", NULL, &generate_bin);
            ImGui::MenuItem("Deploy", NULL, &deploy);
            ImGui::MenuItem("Clean", NULL, &clean);
            ImGui::MenuItem("Fetch", NULL, &validate);

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