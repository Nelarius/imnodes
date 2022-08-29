#pragma once 

#include <imgui.h>
#include <imnodes.h>
#include "block.h"
#include "graph.h"

#include <vector>
#include <map>
#include <string>

using namespace std;

class Context {

public:

    Graph m_graph;

    std::string           system_name;
    std::string           target_ip_address;
    bool                  chirp_enabled = false;
    bool                  trueplay_enabled = false;
    bool                  serialize_protobuf = false;

    Context();
    //~Context();
    void init();
    void loadContext();     // to be defined when the reverse deployment process is implemented
    void buildGraph();
};