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
struct Color3
{
    float data[3];

    Color3() : data{0.0f, 0.0f, 0.0f} {}
};

inline int make_id(int node, int attribute) { return (node << 16) | attribute; }

class SimpleNodeEditor
{
public:
    SimpleNodeEditor()
        : current_id_(0), links_(), number1_(0.0f), number2_(1.0f), color_()
    {
    }

    void show()
    {
        ImGui::Begin("Simple node example");
        ImGui::Text("X -- delete selected link");
        imnodes::BeginNodeEditor();

        {
            const int dragfloat_node_id = 0;
            const float node_width = 150.0f;
            imnodes::BeginNode(dragfloat_node_id);
            imnodes::Name("Drag float");

            // input attributes should all be unique among each other
            imnodes::BeginInputAttribute(make_id(dragfloat_node_id, 0));
            ImGui::Text("input");
            imnodes::EndAttribute();
            ImGui::Spacing();

            {
                // If you don't want a a pin drawn on a node UI element, just
                // don't call Begin(Input|Output)Attribute before and after your
                // UI element!
                const float label_width = ImGui::CalcTextSize("number").x;
                ImGui::Text("number");
                ImGui::PushItemWidth(node_width - label_width - 6.0f);
                ImGui::SameLine();
                ImGui::DragFloat("##hidelabel", &number1_, 0.01f);
                ImGui::PopItemWidth();
            }

            {
                imnodes::BeginOutputAttribute(make_id(dragfloat_node_id, 2));
                const float label_width = ImGui::CalcTextSize("output").x;
                ImGui::Indent(node_width - label_width - 1.5f);
                ImGui::Text("output");
                imnodes::EndAttribute();
            }

            imnodes::EndNode();
        }

        {
            const int big_node_id = 1;
            const float node_width = 200.0f;
            imnodes::BeginNode(big_node_id);
            imnodes::Name("Big node");

            imnodes::BeginInputAttribute(make_id(big_node_id, 0));
            ImGui::Text("input1");
            imnodes::EndAttribute();

            ImGui::Spacing();

            imnodes::BeginInputAttribute(make_id(big_node_id, 1));
            ImGui::Text("input2");
            imnodes::EndAttribute();

            {
                imnodes::BeginOutputAttribute(make_id(big_node_id, 2));
                const float label_width = ImGui::CalcTextSize("number").x;
                ImGui::PushItemWidth(node_width - label_width - 6.0f);
                ImGui::DragFloat("number", &number2_, 0.01f);
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }
            ImGui::Spacing();
            {
                imnodes::BeginOutputAttribute(make_id(big_node_id, 3));
                const float label_width = ImGui::CalcTextSize("color").x;
                ImGui::PushItemWidth(node_width - label_width - 6.0f);
                ImGui::ColorEdit3("color", color_.data);
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }
            ImGui::Spacing();
            {
                imnodes::BeginOutputAttribute(make_id(big_node_id, 5));
                const float label_width = ImGui::CalcTextSize("output1").x;
                ImGui::Indent(node_width - label_width - 1.5f);
                ImGui::Text("output1");
                imnodes::EndAttribute();
            }
            ImGui::Spacing();
            {
                imnodes::BeginOutputAttribute(make_id(big_node_id, 6));
                const float label_width = ImGui::CalcTextSize("output2").x;
                ImGui::Indent(node_width - label_width - 1.5f);
                ImGui::Text("output2");
                imnodes::EndAttribute();
            }

            imnodes::EndNode();
        }

        for (const auto linkpair : links_)
        {
            imnodes::Link(
                linkpair.first, linkpair.second.start, linkpair.second.end);
        }

        imnodes::EndNodeEditor();

        int link_start, link_end;
        if (imnodes::IsLinkCreated(&link_start, &link_end))
        {
            links_.insert(
                std::make_pair(current_id_++, Link{link_start, link_end}));
        }

        int link_id;
        if (imnodes::IsLinkSelected(&link_id))
        {
            if (ImGui::IsKeyReleased(SDL_SCANCODE_X))
            {
                links_.erase(link_id);
            }
        }

        ImGui::End();
    }

private:
    struct Link
    {
        int start, end;
    };

    int current_id_;
    std::unordered_map<int, Link> links_;

    // UI state
    float number1_;
    float number2_;
    Color3 color_;
};

static SimpleNodeEditor editor;
} // namespace

void NodeEditorInitialize()
{
    imnodes::SetNodePos(0, ImVec2(100.0f, 100.0f));
    imnodes::SetNodePos(1, ImVec2(300.0f, 300.0f));
}

void NodeEditorShow() { editor.show(); }

void NodeEditorShutdown() {}

} // namespace example
