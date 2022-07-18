#include "context.h"

Context::Context() {}

/*Context::~Context() {
    ImNodes::EditorContextFree(m_context);
}*/

void Context::init() {
    m_context = ImNodes::EditorContextCreate();
    ImNodes::EditorContextSet(m_context);
}

void Context::loadContext() { }

void Context::update() {
        const int block_id = ++current_block_id;
        ImNodes::SetNodeScreenSpacePos(block_id, ImGui::GetMousePos());
        ImNodes::SnapNodeToGrid(block_id);  //add to canvas
        _blocks.push_back(Block(block_id)); // load names from block library
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