#include "node_editor.h"
#include "imnodes.h"
#include "imgui.h"

#include <SDL_keycode.h>
#include <unordered_map>
#include <utility>

namespace example
{
namespace
{
class SimpleNodeEditor
{
public:
    void show()
    {
        ImGui::Begin("simple node editor");

        imnodes::BeginNodeEditor();
        imnodes::BeginNode(1);

        imnodes::BeginInputAttribute(2);
        ImGui::Text("input");
        imnodes::EndAttribute();

        imnodes::BeginOutputAttribute(3);
        ImGui::Indent(40);
        ImGui::Text("output");
        imnodes::EndAttribute();

        imnodes::EndNode();
        imnodes::EndNodeEditor();

        ImGui::End();
    }
};

static SimpleNodeEditor editor;
} // namespace

void NodeEditorInitialize()
{
    // Give the node a name which is displayed in the title bar.
    // This is state which doesn't change between frames -- this state is
    // retained between calls to BeginNode/EndNode, so there is no need to call
    // these every frame.
    imnodes::SetNodeName(1, "simple node");
    imnodes::SetNodeGridSpacePos(1, ImVec2(200.0f, 200.0f));
}

void NodeEditorShow() { editor.show(); }

void NodeEditorShutdown() {}

} // namespace example
