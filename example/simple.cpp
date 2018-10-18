#include "node_editor.h"
#include "imnodes.h"

#include <vector>

namespace example
{
namespace
{
inline int make_id(int node, int attribute) { return (node << 16) | attribute; }

class SimpleNodeEditor
{
public:
    SimpleNodeEditor()
        : current_id_(0), red_nodes_(), green_nodes_(), blue_nodes_()
    {
    }

    void show()
    {
        ImGui::Begin("Simple node example");
        imnodes::BeginNodeEditor();

        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBar, IM_COL32(255, 30, 30, 255));
        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBarHovered, IM_COL32(255, 80, 80, 255));
        for (int red : red_nodes_)
        {
            const float node_width = 150.0f;
            const float node_padding = 8.0f;
            imnodes::BeginNode(red);
            imnodes::Name("Red");

            imnodes::BeginAttribute(
                make_id(red, 0), imnodes::AttributeType_Input);
            ImGui::TextUnformatted("input");
            imnodes::EndAttribute();

            ImGui::Dummy(ImVec2(node_width, 1.f));

            imnodes::BeginAttribute(
                make_id(red, 1), imnodes::AttributeType_Output);
            const auto label_size = ImGui::CalcTextSize("output");
            ImGui::Indent(node_width - label_size.x - node_padding);
            ImGui::TextUnformatted("output");
            imnodes::EndAttribute();

            imnodes::EndNode();
        }
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBarHovered);
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBar);

        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBar, IM_COL32(30, 255, 30, 255));
        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBarHovered, IM_COL32(100, 255, 100, 255));
        for (int green : green_nodes_)
        {
            const float node_width = 80.0f;
            const float node_padding = 8.0f;
            imnodes::BeginNode(green);
            imnodes::Name("Green");

            imnodes::BeginAttribute(
                make_id(green, 0), imnodes::AttributeType_Input);
            ImGui::TextUnformatted("input");
            imnodes::EndAttribute();

            {
                imnodes::BeginAttribute(
                    make_id(green, 1), imnodes::AttributeType_Internal);
                const float label_width = ImGui::CalcTextSize("color").x;
                ImGui::TextUnformatted("color");
                ImGui::SameLine();
                ImGui::PushItemWidth(node_width - label_width - node_padding);
                // TODO: figure out where to store the color values
                // ImGui::ColorEdit3("##hidelabel", );
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }

            {
                imnodes::BeginAttribute(
                    make_id(green, 1), imnodes::AttributeType_Output);
                const auto label_size = ImGui::CalcTextSize("output").x;
                ImGui::Indent(node_width - label_size - node_padding);
                ImGui::TextUnformatted("output");
                imnodes::EndAttribute();
            }

            imnodes::EndNode();
        }
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBarHovered);
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBar);

        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBar, IM_COL32(30, 30, 255, 255));
        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBarHovered, IM_COL32(80, 80, 255, 255));
        for (int blue : blue_nodes_)
        {
            const float node_width = 100.0f;
            const float node_padding = 8.0f;
            imnodes::BeginNode(blue);
            imnodes::Name("Blue");

            imnodes::BeginAttribute(
                make_id(blue, 0), imnodes::AttributeType_Input);
            ImGui::TextUnformatted("asdf");
            imnodes::EndAttribute();

            {
                imnodes::BeginAttribute(
                    make_id(blue, 1), imnodes::AttributeType_Internal);
                const float label_width = ImGui::CalcTextSize("vector").x;
                ImGui::TextUnformatted("vector");
                ImGui::SameLine();
                ImGui::PushItemWidth(node_width - label_width - node_padding);
                // TODO:
                // ImGui::DragFloat3("##hidelabel");
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }

            {
                imnodes::BeginAttribute(
                    make_id(blue, 2), imnodes::AttributeType_Output);
                const float label_width = ImGui::CalcTextSize("asdfasdf").x;
                ImGui::Indent(node_width - label_width);
                ImGui::TextUnformatted("asdfasdf");
                imnodes::EndAttribute();
            }

            imnodes::EndNode();
        }
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBarHovered);
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBar);

        // Context menu for adding new nodes

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));

        if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseHoveringWindow() &&
            ImGui::IsMouseClicked(1))
        {
            ImGui::OpenPopup("context menu");
        }

        if (ImGui::BeginPopup("context menu"))
        {
            int new_node = -1;
            ImVec2 click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();

            if (ImGui::MenuItem("Red"))
            {
                new_node = current_id_++;
                red_nodes_.push_back(new_node);
            }

            if (ImGui::MenuItem("Blue"))
            {
                new_node = current_id_++;
                blue_nodes_.push_back(new_node);
            }

            if (ImGui::MenuItem("Green"))
            {
                new_node = current_id_++;
                green_nodes_.push_back(new_node);
            }

            ImGui::EndPopup();

            if (new_node != -1)
            {
                imnodes::SetNodePos(new_node, click_pos, ImGuiCond_Appearing);
            }
        }
        ImGui::PopStyleVar();
        imnodes::EndNodeEditor();

        ImGui::End();
    }

private:
    int current_id_;
    std::vector<int> red_nodes_;
    std::vector<int> green_nodes_;
    std::vector<int> blue_nodes_;
};

static SimpleNodeEditor editor;
} // namespace

void NodeEditorInitialize() {}

void NodeEditorShow() { editor.show(); }

void NodeEditorShutdown() {}

} // namespace example