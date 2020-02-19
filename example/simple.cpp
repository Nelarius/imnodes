#include "node_editor.h"
#include <imnodes.h>
#include <imgui.h>

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

        imnodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("simple node :)");
        imnodes::EndNodeTitleBar();

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
    imnodes::SetNodeGridSpacePos(1, ImVec2(200.0f, 200.0f));
}

void NodeEditorShow() { editor.show(); }

void NodeEditorShutdown() {}

} // namespace example
