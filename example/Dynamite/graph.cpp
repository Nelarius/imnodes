#include "graph.h"

#include <iostream>
#include <algorithm>
#include <imgui.h>
#include "dyndsp_wrapper.h"

using namespace std;

struct adjlist_node;
struct adjlist;

struct BlockNames names;
struct BlockParameters parameters;

void Graph::init() {
    // Calling dyndsp_wrapper
    names.dsp_names = DyndspWrapper::get_dsp_list();
    names.control_names = DyndspWrapper::get_control_list();

    // Loop dsp names to retrieve each of their parameters
    for (auto dsp_name : names.dsp_names) 
    {
        parameters.parameter_names_for_block.emplace(dsp_name, DyndspWrapper::get_parameter_names(dsp_name));
        parameters.parameter_types_for_block.emplace(dsp_name, DyndspWrapper::get_parameter_types(dsp_name));
    }

    // Loop control names to retrieve each of their parameters
    for (auto control_name : names.control_names)
    {
        parameters.parameter_names_for_block.emplace(control_name, DyndspWrapper::get_parameter_names(control_name));
        parameters.parameter_types_for_block.emplace(control_name, DyndspWrapper::get_parameter_types(control_name));
    }

    // Rendering editor context
    m_context = ImNodes::EditorContextCreate();
    ImNodes::EditorContextSet(m_context);
    
}

/* Blocks */
void Graph::addBlock(std::string blockname) {
    const int block_id = ++current_block_id;
    Block block(block_id, blockname);
    for (int i = 0; i < 2; i++) {
        if (blockname != "input") {
            Port port(current_port_id, "INPUT");
            block.addInPort(current_port_id, port);
            ++current_port_id;
        }
    }
    for (int i = 0; i < 2; i++) {
        if (blockname != "output") {
            Port port(current_port_id, "OUTPUT");
            block.addOutPort(current_port_id, port);
            ++current_port_id;
        }
    }

    std::vector<std::string> parameter_names = parameters.parameter_names_for_block[block.getType()];
    std::vector<std::string> parameter_types = parameters.parameter_types_for_block[block.getType()];
    for (std::vector<std::string>::size_type i = 0; i != parameter_names.size(); i++) {
        Parameter parameter(current_param_id, parameter_names[i], parameter_types[i]);
        block.addParam(current_param_id, parameter);
        ++current_param_id;
    }

    ImNodes::SetNodeScreenSpacePos(block_id, ImVec2(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y / 2));
    ImNodes::SnapNodeToGrid(block_id);  // add to canvas
    _blocks.push_back(block); // load names from block library
}

void Graph::deleteBlock(int node_id) {
    ImNodes::ClearNodeSelection(node_id);
    auto iter = std::find_if(
        _blocks.begin(), _blocks.end(), [node_id](Block& block) -> bool {
            return block.getID() == node_id;
        });
        assert(iter != _blocks.end());
        _blocks.erase(iter);
}

void Graph::clearBlocks() {
    _blocks.clear();
}

/* Links */
void Graph::addLink() {
    Link link;
    if (ImNodes::IsLinkCreated(&link.start_attr, &link.end_attr)) {
        // Check if the end attribute already has a link
        // If so, delete the old link
        if ((int)_links.size() > 0) {
            auto iter = std::find_if(
                _links.begin(), _links.end(), [link](const Link& temp) -> bool {
                    return temp.end_attr == link.end_attr;
                });
            if (iter != _links.end()){
                // Unlink the reference of output port its linked input port(s)
                for (auto &block : _blocks) {
                    for (auto &port : block._inPorts) {
                        if (port.first == iter->end_attr) {
                            port.second.reference_name = nullptr;
                        }
                    }
                }
                _links.erase(iter);
            }
        }

        link.id = ++current_link_id;
        _links.push_back(link);

        // Make a reference from output ports to other block's input port(s)
        static char* temp;
        for (auto &block : _blocks) {
            for (auto &port : block._outPorts) {
                if (port.first == link.start_attr) {
                    temp = (char*)port.second.name;
                }
            }
        }
        for (auto &block : _blocks) {
            for (auto &port : block._inPorts) {
                if (port.first == link.end_attr) {
                    port.second.reference_name = temp;
                }
            }
        }
    }
}

void Graph::deleteLink(int link_id) {
    if (ImNodes::IsLinkDestroyed(&link_id)) {
        auto iter = std::find_if(
            _links.begin(), _links.end(), [link_id](const Link& link) -> bool {
                return link.id == link_id;
            });
        assert(iter != _links.end());

        // Unlink the reference of output port its linked input port(s)
        for (auto &block : _blocks) {
            for (auto &port : block._inPorts) {
                if (port.first == iter->end_attr) {
                    port.second.reference_name = nullptr;
                }
            }
        }
        _links.erase(iter);
    }
}

void Graph::clearLinks() {
    _links.clear();
}

/* Adjacency list */

adjlist_node* Graph::newNode(int dest) {
    adjlist_node* newNode = new adjlist_node;
    for (auto& b : _blocks) {
        if (b.getID() == dest) {
            newNode->block = b;
        } else {
            newNode->block = NULL;
        }
    }
    newNode->dest = dest;
    newNode->next = NULL;
    return newNode;
}

// buildAdjacencyList helper function
bool portIterator(Block& b, std::vector<Link>::iterator link_iter) {
    for (auto& p : b._outPorts) {
        if (p.first == link_iter->start_attr) {
            return true;
        } else {
            continue;
        }
    }
    return false;
}

void Graph::buildAdjacencyList() {
    // when num_vertices is grabbed as _blocks.size()
    this->num_vertices = _blocks.back().getID() + 1;
    // initialize list
    array = new adjlist[num_vertices];
    for (int i = 0; i<num_vertices; i++) {
        array[i].head = NULL;
    }

    for (auto& block : _blocks) {
        for (auto& port : block._inPorts) {
            auto link_iter = std::find_if(
                _links.begin(), _links.end(), [port](const Link& temp) -> bool {
                    return port.first == temp.end_attr;
            });
            if (link_iter != _links.end()) {
                auto block_iter = std::find_if(
                    _blocks.begin(), _blocks.end(), [link_iter](Block& b) -> bool {
                        return portIterator(b, link_iter);
                });
                if (block_iter != _blocks.end()) {
                    if (!Graph::containsEdge(block_iter->getID(), block.getID())) {
                        Graph::addEdge(block_iter->getID(), block.getID());
                    }
                } 
            }
        }
    }
}

void Graph::addEdge(int src, int dest) {
    adjlist_node* new_node = newNode(dest);
    new_node->next = array[src].head;
    array[src].head = new_node;
}

bool Graph::containsEdge(int src, int dest) {
    for (adjlist_node* node = array[src].head; node; node = node->next) {
        if (node->dest == dest) {
            return true;
        }
    }
    return false;
}

void Graph::display() {
    int v;
    for (v = 0; v < num_vertices; ++v)
    {
        adjlist_node* pCrawl = array[v].head;
        cout<<"\n Adjacency list of vertex "<<v<<"\n head ";
        while (pCrawl)
        {
            std::string blockname = pCrawl->block.getName();
            cout << "-> " << pCrawl->dest;
            pCrawl = pCrawl->next;
        }
        cout << endl;
    }
}

/* Sort */

// TO DO : this works for simple parallel paths that rejoin, are there any edge cases? 
int Graph::findStartNode() {
    std::vector<int> pathLens(num_vertices);
    for (int v = 0; v < num_vertices; ++v) {
        adjlist_node* pCrawl = array[v].head;

        int len = 0;
        while(pCrawl) {
            len++;
            pCrawl = array[pCrawl->dest].head;
        }
        pathLens.at(v) = len;
    }
    int max_index = max_element(pathLens.begin(), pathLens.end()) - pathLens.begin();
    return max_index;
}

void Graph::topologicalSortHelper(int v, bool visited[], std::stack<int>& _stack) {
    visited[v] = true;
    printf("visited node : %d\n", v);
    adjlist_node* dep = array[v].head;
    while(dep) {
        if (!visited[dep->dest]) {
            topologicalSortHelper(dep->dest, visited, _stack);
        }
        dep = dep->next;
    }
    _stack.push(v);
}

void Graph::topologicalSort() {
    int max_index = Graph::findStartNode(); 
    int curr_index = max_index;

    std::stack<int> _stack;
    bool* visited = new bool[num_vertices];
    for (int i = 0; i < num_vertices; i++) {
        visited[i] = false;
    }

    adjlist_node* pCrawl = array[max_index].head; // the first block in the graph
    while (pCrawl) {
        if (visited[curr_index] == false) {
            printf("calling helper on %d\n", curr_index);
            topologicalSortHelper(curr_index, visited, _stack);
        }
        pCrawl = pCrawl->next;
    }

    while (_stack.empty() == false) {
        cout << _stack.top() << " ";
        _stack.pop();
    }
    cout << endl;
}