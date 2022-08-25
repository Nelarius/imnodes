#include "context.h"
#include "dyndsp_wrapper.h"
#include <SDL_scancode.h>
#include <iostream>

struct BlockNames names;
struct BlockParameters parameters;

Context::Context() {
}

/*Context::~Context() {
    ImNodes::EditorContextFree(m_context);
}*/

void Context::init() {
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

void Context::loadContext() { }

void Context::update(bool add, std::string blockname) {
    if (add) // add a block
    {
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
    else // delete a block
    {
        int nodeid = std::stoi(blockname);
        ImNodes::ClearNodeSelection(nodeid);
        deleteBlock(nodeid);
    }
}

bool port_iterator(Block& b, std::vector<Link>::iterator link_iter) {
    for (auto& p : b._inPorts) {
        printf("comparing link end attr %d to port id %d on block %d\n", link_iter->end_attr, p.first, b.getID());
        if (p.first == link_iter->end_attr) {
            return true;
        } else {
            continue;
        }
    }
    return false;
}

void Context::buildGraph() {
    // initialize graph
    m_graph = Graph(_blocks.back().getID() + 1); 

    for (auto& block : _blocks) {
        for (auto& port : block._outPorts) {
            auto link_iter = std::find_if(
                _links.begin(), _links.end(), [port](const Link& temp) -> bool {
                    return port.first == temp.start_attr;
            });
            if (link_iter != _links.end()) {
                auto block_iter = std::find_if(
                    _blocks.begin(), _blocks.end(), [link_iter](Block& b) -> bool {
                            return port_iterator(b, link_iter);
                });
                if (block_iter != _blocks.end()) {
                    if (!m_graph.contains_edge(block.getID(), block_iter->getID())) {
                        m_graph.add_edge(block.getID(), block_iter->getID());
                        // move on to serialization by breadth first
                        // when a branch diverges, print block up until the path unites again from where it broke off, a
                        // then go back to the reference where it splits and print those until the convergence point
                    }
                } 
            }         
        }
        printf("\n\n");
    }
    m_graph.display();
}

void Context::deleteBlock(int node_id) {
    auto iter = std::find_if(
        _blocks.begin(), _blocks.end(), [node_id](Block& block) -> bool {
            return block.getID() == node_id;
        });
        assert(iter != _blocks.end());
        _blocks.erase(iter);
}

void Context::addLink() {
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

void Context::deleteLink(int link_id) {
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