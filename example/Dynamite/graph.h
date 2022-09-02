#pragma once

#include <list>
#include <string>
#include <vector>
#include <stack>

#include <imnodes.h>
#include "block.h"

// Link struct
struct Link 
{
    int id;
    int start_attr, end_attr;
};

// adjacency list helper structs
// TO DO : do we need two separate structs for this linked list type struct
struct adjlist_node {
    Block block;
    int dest;
    struct adjlist_node* next;
};
struct adjlist {
    struct adjlist_node* head;
};

// DynDSP helper structs
struct BlockNames 
{
    std::vector<std::string> dsp_names, control_names;
};
struct BlockParameters
{
    std::map<std::string, std::vector<std::string>> parameter_names_for_block;
    std::map<std::string, std::vector<std::string>> parameter_types_for_block;
};

class Graph {

public: 
    std::vector<Block>  _blocks;
    std::vector<Link>   _links;
    // TO DO : delete member stacks and have adjlist/sort functions return them instead 
    std::stack<int>     blockid_stack;
    std::stack<Block>   block_stack;

    int                   current_link_id = 0;
    int                   current_block_id = 0;
    int                   current_port_id = 0;
    int                   current_param_id = 0;

    Graph() = default;
    Graph(int v) : num_vertices{ v } {
        array = new adjlist[v];
        for (int i = 0; i<v; i++) {
            array[i].head = NULL;
        }
    }
    // Initialize the ImNodes::EditorContext ( TO DO : move this action to the editor)
    // Calls to the dyndspwrapper for getting block name and parameters data
    void init();

    void addBlock(std::string blockname);
    void deleteBlock(int node_id);
    Block findBlock(int id);
    void clearBlocks();

    void addLink();
    void deleteLink(int link_id);
    void clearLinks();

    void buildAdjacencyList();
    adjlist_node* newNode(int dest);
    void addEdge(int src, int dest);
    void display();
    bool containsEdge(int src, int dest);

    void topologicalSortHelper(int v, bool visited[]);
    void topologicalSort(); // change return type to vector<Block>
    int findStartNode();

private:
    int                 num_vertices;
    struct adjlist*     array;

    ImNodesEditorContext* m_context = nullptr;
};