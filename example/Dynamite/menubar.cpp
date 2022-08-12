#include "menubar.h"

static bool command = false;
static void validate();

CPyInstance hInst;

void MenuBar::show() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            vector<std::string> menuItems { "New", "Open", "Save", "Save as", "Import", "Export", "Close"  };
            //createMenu(menuItems);
            for (auto Item : menuItems) {
                if (ImGui::MenuItem(Item.c_str(), NULL)) {
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

        if (command) validate();

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

static void validate() {
    // Provide path for Python to find file
    PyRun_SimpleString("import sys");
    
    // Find a Python file named run_test.py
    CPyObject pName = PyUnicode_FromString("sonos.audio.dynamicdsp.commands.v1alpha1");
    CPyObject pModule = PyImport_Import(pName);

    if (pModule) {
        printf("module accessed\n");
        CPyObject pCommand = PyObject_GetAttrString(pModule, "validate");

        if (pCommand && PyCallable_Check(pCommand)) {
            printf("can open command\n");
            //Create argument to send over
            CPyObject pArg = PySys_GetObject("system.json");
            CPyObject pRules = PyObject_CallObject(pCommand, pArg);

            auto listRulesSize = PyList_Size(pRules);
            for (Py_ssize_t j = 0 ; j < listRulesSize; ++j)
            {
                PyObject* rules = PyList_GetItem(pRules, j);
                string rule(PyUnicode_AsUTF8(rules));
                printf("Rules : %s\n", rule.c_str());
            }
        }
    }
    command = false;
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