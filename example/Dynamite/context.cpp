#include "context.h"
#include "dyndsp_wrapper.h"
#include <SDL_scancode.h>

// Retrieve from DyndspWrapper
//extern struct DynDSPData;

struct BlockNames names;
struct BlockParameters parameters;

Context::Context() {}

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

int Context::addBlock() {
    const int block_id = ++current_block_id;
    _blocks.push_back(Block(block_id, "DSPBlock")); // load names from block library
    return block_id;
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
                    printf("PASSED");
                    temp = (char*)port.second.name;
                    printf("Temp name: %c\n", *(temp));
                }
            }
        }
        for (auto &block : _blocks) {
            for (auto &port : block._inPorts) {
                if (port.first == link.end_attr) {
                    printf("Reference name: %c\n", *(temp));
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