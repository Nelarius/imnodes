#include "context.h"
#include <vector>
#include <map>
#include <string>

struct BlockNames names;
struct BlockParameters parameters;

Context::Context() {}

/*Context::~Context() {
    ImNodes::EditorContextFree(m_context);
}*/

void Context::init() {
    // Calling dyndsp_wrapper
    names.dsp_names = m_wrapper.get_dsp_list();
    names.control_names = m_wrapper.get_control_list();

    // Loop dsp names to retrieve each of their parameters
    for (auto dsp_name : names.dsp_names) 
    {
        parameters.parameter_list.emplace(dsp_name, m_wrapper.get_parameters(dsp_name));
    }

    // Loop control names to retrieve each of their parameters
    for (auto control_name : names.control_names)
    {
        parameters.parameter_list.emplace(control_name, m_wrapper.get_parameters(control_name));
    }

    // Rendering editor context
    m_context = ImNodes::EditorContextCreate();
    ImNodes::EditorContextSet(m_context);
}

void Context::loadContext() { }

void Context::update(bool add, string blockname) {
    if (add) // add a block
    {
        const int block_id = ++current_block_id;
        Block block(block_id, blockname);
        for (int i = 0; i < 2; i++) {
            Port port(current_port_id, "INPUT");
            (block._inPorts).insert({current_port_id, port});
            ++current_port_id;
        }
        for (int i = 0; i < 2; i++) {
            Port port(current_port_id, "OUTPUT");
            (block._outPorts).insert({current_port_id, port});
            ++current_port_id;
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
        link.id = ++current_link_id;
        _links.push_back(link);
    }
}

void Context::deleteLink(int link_id) {
    if (ImNodes::IsLinkDestroyed(&link_id)) {
        auto iter = std::find_if(
            _links.begin(), _links.end(), [link_id](const Link& link) -> bool {
                return link.id == link_id;
            });
        assert(iter != _links.end());
        _links.erase(iter);
    }
}