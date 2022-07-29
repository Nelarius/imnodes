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

void Context::update(string blockname) {
    const int block_id = ++current_block_id;
    ImNodes::SetNodeScreenSpacePos(block_id, ImVec2(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y / 2));
    ImNodes::SnapNodeToGrid(block_id);  //add to canvas
    if (blockname == "") {
        _blocks.push_back(Block(block_id));
    } else {
        // hard coded for the demo to have a single input and output channels
        _blocks.push_back(Block(block_id, blockname, "IN", "OUT")); // load names from block library
    }
}

int Context::addBlock() {
    const int block_id = ++current_block_id;
    _blocks.push_back(Block(block_id, "DSPBlock", "IN", "OUT")); // load names from block library
    return block_id;
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