#pragma once

#include <vector>
#include <string>
#include "context.h"
#include "JsonGraphFileWriter.h"
#include "dyndsp_wrapper.h"

using namespace std;

struct MenuAction {
    std::string name; 
};

struct SubMenu {
    std::string name;
    std::vector<MenuAction> menuItems; 
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

class MenuBar {
public:
    void show(Context &m_context, DyndspWrapper m_wrapper);
    //void createMenu(vector<std::string> menuItems);

};