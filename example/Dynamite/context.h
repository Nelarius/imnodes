#include <imgui.h>
#include <imnodes.h>
#include "block.h"

#include <SDL_scancode.h>
#include <vector>

using namespace std;

struct Link {
    int id;
    int start_attr, end_attr;
};

class Context {

    std::vector<Block>    blocks;
    std::vector<Link>     links;
    int                   current_block_id = 0;
    int                   current_link_id = 0;

    public: 

    Context() { 
        /* context = ImNodes::EditorContextCreate();
        ImNodes::EditorContextSet(context);  */
    }

    void displayInEditor() {
        for (Block& block : blocks) {
            block.show();
        } 

        for (const Link& link : links) {
            ImNodes::Link(link.id, link.start_attr, link.end_attr);
        }
    }

    void addBlock() {
            const int block_id = ++current_block_id;
            blocks.push_back(Block(block_id, "DSPBlock", "IN", "OUT")); // load names from block library
    }

    void addLink() {
        Link link;
        if (ImNodes::IsLinkCreated(&link.start_attr, &link.end_attr)) {
            link.id = ++current_link_id;
            links.push_back(link);
        }
    }

    void deleteLink(int link_id) {
        if (ImNodes::IsLinkDestroyed(&link_id)) {
            auto iter = std::find_if(
                links.begin(), links.end(), [link_id](const Link& link) -> bool {
                    return link.id == link_id;
                });
            assert(iter != links.end());
            links.erase(iter);
        }
    }


};