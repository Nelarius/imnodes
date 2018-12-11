#include "node_editor.h"
#include "imnodes.h"
#include "imgui.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace example
{
namespace
{
struct Color3
{
    float data[3];
};

inline int make_id(int node, int attribute) { return (node << 16) | attribute; }

class SimpleNodeEditor
{
public:
    SimpleNodeEditor()
        : current_id_(0), node_array_map_(), input_nodes_(), output_nodes_(),
          dragfloat_nodes_(), big_nodes_(), dragfloat_data_(), color_data_()
    {
    }

    void show()
    {
        ImGui::Begin("Simple node example");
        imnodes::BeginNodeEditor();

        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBar, IM_COL32(170, 88, 57, 255));
        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBarHovered, IM_COL32(212, 135, 106, 255));
        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBarSelected, IM_COL32(255, 193, 170, 255));
        for (int input_node : input_nodes_)
        {
            imnodes::BeginNode(input_node);
            imnodes::Name("Input");
            imnodes::BeginAttribute(
                make_id(input_node, 0), imnodes::AttributeType_Input);
            ImGui::Text("input");
            imnodes::EndAttribute();
            imnodes::EndNode();
        }
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBarSelected);
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBarHovered);
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBar);

        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBar, IM_COL32(170, 121, 57, 255));
        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBarHovered, IM_COL32(212, 166, 106, 255));
        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBarSelected, IM_COL32(255, 218, 170, 255));
        for (int output_node : output_nodes_)
        {
            imnodes::BeginNode(output_node);
            imnodes::Name("Output");
            imnodes::BeginAttribute(
                make_id(output_node, 0), imnodes::AttributeType_Output);
            ImGui::Text("output");
            imnodes::EndAttribute();
            imnodes::EndNode();
        }
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBarSelected);
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBarHovered);
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBar);

        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBar, IM_COL32(41, 81, 109, 255));
        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBarHovered, IM_COL32(72, 109, 136, 255));
        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBarSelected, IM_COL32(112, 142, 164, 255));
        for (int dragfloat_node : dragfloat_nodes_)
        {
            const float node_width = 150.0f;
            imnodes::BeginNode(dragfloat_node);
            imnodes::Name("Drag float");

            imnodes::BeginAttribute(
                make_id(dragfloat_node, 0), imnodes::AttributeType_Input);
            ImGui::Text("input");
            imnodes::EndAttribute();
            ImGui::Spacing();

            {
                // use an internal attribute when you don't want a pin to be
                // drawn on the attribute
                imnodes::BeginAttribute(
                    make_id(dragfloat_node, 1),
                    imnodes::AttributeType_Internal);
                const float label_width = ImGui::CalcTextSize("number").x;
                ImGui::Text("number");
                ImGui::PushItemWidth(node_width - label_width - 6.0f);
                ImGui::SameLine();
                ImGui::DragFloat(
                    "##hidelabel", &dragfloat_data_[dragfloat_node], 0.01f);
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }

            {
                imnodes::BeginAttribute(
                    make_id(dragfloat_node, 2), imnodes::AttributeType_Output);
                const float label_width = ImGui::CalcTextSize("output").x;
                ImGui::Indent(node_width - label_width - 1.5f);
                ImGui::Text("output");
                imnodes::EndAttribute();
            }

            imnodes::EndNode();
        }
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBarSelected);
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBarHovered);
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBar);

        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBar, IM_COL32(39, 117, 82, 255));
        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBarHovered, IM_COL32(73, 147, 113, 255));
        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBarSelected, IM_COL32(117, 176, 149, 255));
        for (int big_node : big_nodes_)
        {
            const float node_width = 200.0f;
            imnodes::BeginNode(big_node);
            imnodes::Name("Big node");

            imnodes::BeginAttribute(
                make_id(big_node, 0), imnodes::AttributeType_Input);
            ImGui::Text("input1");
            imnodes::EndAttribute();

            ImGui::Spacing();

            imnodes::BeginAttribute(
                make_id(big_node, 1), imnodes::AttributeType_Input);
            ImGui::Text("input2");
            imnodes::EndAttribute();

            {
                imnodes::BeginAttribute(
                    make_id(big_node, 2), imnodes::AttributeType_Output);
                const float label_width = ImGui::CalcTextSize("number").x;
                ImGui::PushItemWidth(node_width - label_width - 6.0f);
                ImGui::DragFloat("number", &dragfloat_data_[big_node], 0.01f);
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }
            ImGui::Spacing();
            {
                imnodes::BeginAttribute(
                    make_id(big_node, 3), imnodes::AttributeType_Output);
                const float label_width = ImGui::CalcTextSize("color").x;
                ImGui::PushItemWidth(node_width - label_width - 6.0f);
                ImGui::ColorEdit3("color", color_data_[big_node].data);
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }
            ImGui::Spacing();
            {
                imnodes::BeginAttribute(
                    make_id(big_node, 5), imnodes::AttributeType_Output);
                const float label_width = ImGui::CalcTextSize("output1").x;
                ImGui::Indent(node_width - label_width - 1.5f);
                ImGui::Text("output1");
                imnodes::EndAttribute();
            }
            ImGui::Spacing();
            {
                imnodes::BeginAttribute(
                    make_id(big_node, 6), imnodes::AttributeType_Output);
                const float label_width = ImGui::CalcTextSize("output2").x;
                ImGui::Indent(node_width - label_width - 1.5f);
                ImGui::Text("output2");
                imnodes::EndAttribute();
            }

            imnodes::EndNode();
        }
        imnodes::PopColorStyle(imnodes::ColorStyle_TitleBarSelected);
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

            if (ImGui::MenuItem("Input only"))
            {
                new_node = current_id_++;
                input_nodes_.push_back(new_node);
                node_array_map_[new_node] = &input_nodes_;
            }

            if (ImGui::MenuItem("Output only"))
            {
                new_node = current_id_++;
                output_nodes_.push_back(new_node);
                node_array_map_[new_node] = &output_nodes_;
            }

            if (ImGui::MenuItem("Drag float"))
            {
                new_node = current_id_++;
                dragfloat_nodes_.push_back(new_node);
                dragfloat_data_[new_node] = 1.0f;
                node_array_map_[new_node] = &dragfloat_nodes_;
            }

            if (ImGui::MenuItem("Big node"))
            {
                new_node = current_id_++;
                dragfloat_data_[new_node] = 0.0f;
                color_data_[new_node] = Color3{1.0f, 0.0f, 0.0f};
                big_nodes_.push_back(new_node);
                node_array_map_[new_node] = &big_nodes_;
            }

            ImGui::EndPopup();

            if (new_node != -1)
            {
                imnodes::SetNodePos(new_node, click_pos, ImGuiCond_Appearing);
            }
        }
        ImGui::PopStyleVar();
        imnodes::EndNodeEditor();

        // event handling
        {
            imnodes::Event event;
            while (imnodes::PollEvent(event))
            {
                switch (event.type)
                {
                    case imnodes::EventType_NodeDeleted:
                    {
                        auto& nodes =
                            *node_array_map_[event.node_deleted.node_idx];
                        std::swap(
                            *std::find(
                                nodes.begin(),
                                nodes.end(),
                                event.node_deleted.node_idx),
                            nodes.back());
                        nodes.pop_back();
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        ImGui::End();
    }

private:
    int current_id_;

    std::unordered_map<int, std::vector<int>*> node_array_map_;
    std::vector<int> input_nodes_;
    std::vector<int> output_nodes_;
    std::vector<int> dragfloat_nodes_;
    std::vector<int> big_nodes_;

    std::unordered_map<int, float> dragfloat_data_;
    std::unordered_map<int, Color3> color_data_;
};

static SimpleNodeEditor editor;
} // namespace

void NodeEditorInitialize() {}

void NodeEditorShow() { editor.show(); }

void NodeEditorShutdown() {}

} // namespace example
