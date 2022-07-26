#include "menubar.h"

struct MenuAction {
    string name; 
    //lambda menuAction; 
};

struct SubMenu {
    string name;
    vector<MenuAction> menuItems; 
};

// menubar owns static copies of the structs
// functions may not be static couz they are passed in at runtime
// or use a function pointer
/* 
vector<MenuActions> = { name, action }

vector<SubMenus>
*/

// document the editorcontext use cases into the google doc
// creating subsystems, lives only in the app side of things, but not the code 
// only the UI knows its a subsystem, click on it and see the context contained


/*
- add channel/delete channel button on multi purpose panel
default each block to 2 input and output channels 
*/ 

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

        if (ImGui::BeginMenu("Commands")) {
            vector<std::string> menuItems { "Validate", "Generate", "Fetch", "Deploy", "Clean" };
            //createMenu(menuItems);
            for (auto Item : menuItems) {
                if (ImGui::MenuItem(Item.c_str(), NULL)) {
                    printf("menu opened!\n");
                }
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

/*
void createMenu(vector<std::string> menuItems) {
    for (auto Item : menuItems) {
        if (ImGui::MenuItem(Item.c_str(), NULL)) {
            printf("menu opened!\n");
        }
    }
}
*/