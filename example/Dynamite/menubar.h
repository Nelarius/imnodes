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

class MenuBar {
public:
    void show(Context &m_context, DyndspWrapper m_wrapper);
    //void createMenu(vector<std::string> menuItems);

};