#pragma once 

#include <imgui.h>
#include <imnodes.h>
#include "block.h"
#include "dyndsp_wrapper.h"

#include <SDL_scancode.h>
#include <vector>
#include <map>
#include <string>

using namespace std;

struct BlockNames 
{
    std::vector<std::string> dsp_names, control_names;
};

struct BlockParameters
{
    std::map<std::string, std::vector<std::string>> parameter_names_for_block;
    std::map<std::string, std::vector<std::string>> parameter_types_for_block;
};

struct Link 
{
    int id;
    int start_attr, end_attr;
};

class Context {

    
public:

    DyndspWrapper m_wrapper;
    ImNodesEditorContext* m_context = nullptr;
    std::string           system_name;
    std::vector<Block>    _blocks;
    std::vector<Link>     _links;
    int                   current_link_id = 0;
    int                   current_block_id = 0;
    int                   current_port_id = 0;
    int                   current_param_id = 0;

        Context();
        //~Context();
        void init();
        void loadContext(); 
        void update(bool add, string blockname);
        int addBlock();
        void deleteBlock(int node_id);
        void addLink();
        void deleteLink(int link_id);

/*
either in the constructor or in an init function, the context will have to take in 
the .JSON file and use that to define all the block ids, link ids
all before being sent to the UI to be displayed in the central canvas

most likely an init function so that that happens on function call, because we might load the app 
on to a blank canvas, then go File->Open->select an instance and load that saved context 
*/


};