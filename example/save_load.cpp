#include "node_editor.h"
#include "imnodes.h"
#include "imgui.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>

namespace example
{
namespace
{
struct Color3
{
    float data[3];
};

inline int make_id(int node, int attribute) { return (node << 16) | attribute; }

class SaveLoadEditor
{
public:
    SaveLoadEditor() : float_nodes_(), color_nodes_() {}

    void show()
    {
        ImGui::Begin("Save & load example");

        imnodes::BeginNodeEditor();

        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBar, IM_COL32(39, 117, 82, 255));
        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBarHovered, IM_COL32(73, 147, 113, 255));
        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBarSelected, IM_COL32(117, 176, 149, 255));
        for (auto& elem : float_nodes_)
        {
            const float node_width = 150.0f;
            imnodes::BeginNode(elem.first);
            imnodes::Name("drag float");

            imnodes::BeginAttribute(
                make_id(elem.first, 0), imnodes::AttributeType_Input);
            ImGui::Text("input");
            imnodes::EndAttribute();
            ImGui::Spacing();
            {
                imnodes::BeginAttribute(
                    make_id(elem.first, 1), imnodes::AttributeType_Internal);
                const float label_width = ImGui::CalcTextSize("number").x;
                ImGui::Text("number");
                ImGui::PushItemWidth(node_width - label_width - 6.0f);
                ImGui::SameLine();
                ImGui::DragFloat("##hidelabel", &elem.second, 0.01f);
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }
            ImGui::Spacing();
            {
                imnodes::BeginAttribute(
                    make_id(elem.first, 2), imnodes::AttributeType_Output);
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
            imnodes::ColorStyle_TitleBar, IM_COL32(41, 81, 109, 255));
        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBarHovered, IM_COL32(72, 109, 136, 255));
        imnodes::PushColorStyle(
            imnodes::ColorStyle_TitleBarSelected, IM_COL32(112, 142, 164, 255));
        for (auto& elem : color_nodes_)
        {
            const float node_width = 200.0f;
            imnodes::BeginNode(elem.first);
            imnodes::Name("color");

            imnodes::BeginAttribute(
                make_id(elem.first, 0), imnodes::AttributeType_Input);
            ImGui::Text("input");
            imnodes::EndAttribute();
            ImGui::Spacing();

            {
                imnodes::BeginAttribute(
                    make_id(elem.first, 3), imnodes::AttributeType_Output);
                const float label_width = ImGui::CalcTextSize("color").x;
                ImGui::PushItemWidth(node_width - label_width - 6.0f);
                ImGui::ColorEdit3("color", elem.second.data);
                ImGui::PopItemWidth();
                imnodes::EndAttribute();
            }
            ImGui::Spacing();
            {
                imnodes::BeginAttribute(
                    make_id(elem.first, 2), imnodes::AttributeType_Output);
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

            if (ImGui::MenuItem("drag float node"))
            {
                new_node = current_id_++;
                float_nodes_.insert(std::make_pair(new_node, 0.f));
            }

            if (ImGui::MenuItem("color node"))
            {
                new_node = current_id_++;
                color_nodes_.insert(std::make_pair(new_node, Color3{}));
            }

            ImGui::EndPopup();

            if (new_node != -1)
            {
                imnodes::SetNodePos(new_node, click_pos, ImGuiCond_Appearing);
            }
        }

        ImGui::PopStyleVar();
        imnodes::EndNodeEditor();

        // imnodes event handling
        {
            imnodes::Event event;
            while (imnodes::PollEvent(event))
            {
                switch (event.type)
                {
                    case imnodes::EventType_NodeDeleted:
                    {
                        if (float_nodes_.find(event.node_deleted.node_idx) !=
                            float_nodes_.end())
                        {
                            float_nodes_.erase(event.node_deleted.node_idx);
                        }
                        else if (
                            color_nodes_.find(event.node_deleted.node_idx) !=
                            color_nodes_.end())
                        {
                            color_nodes_.erase(event.node_deleted.node_idx);
                        }
                        break;
                    }

                    default:
                        break;
                }
            }
        }

        ImGui::End();
    }

    void save()
    {
        {
            std::ofstream fout;
            fout.open("editor.ini");

            for (const auto& elem : float_nodes_)
            {
                fout << "[float-node]\n";
                fout << "id=" << elem.first << "\n";
                fout << "data=" << elem.second << "\n\n";
            }

            for (const auto& elem : color_nodes_)
            {
                fout << "[color-node]\n";
                fout << "id=" << elem.first << "\n";
                fout << "data=" << elem.second.data[0] << ","
                     << elem.second.data[1] << "," << elem.second.data[2]
                     << "\n\n";
            }

            fout.close();
        }

        imnodes::SaveCurrentEditorStateToDisk("imnodes.ini");
    }

    void load()
    {
        {
            std::ifstream fin("editor.ini");
            if (fin.is_open())
            {
                // a simple, stupid ini parser
                std::string line;
                while (std::getline(fin, line))
                {
                    if (line == "[float-node]")
                    {
                        int id;
                        float value;
                        std::getline(fin, line);
                        std::sscanf(line.c_str(), "id=%i", &id);
                        std::getline(fin, line);
                        std::sscanf(line.c_str(), "data=%f", &value);
                        float_nodes_.insert(std::make_pair(id, value));

                        if (id > current_id_)
                        {
                            current_id_ = id;
                        }
                    }
                    else if (line == "[color-node]")
                    {
                        int id;
                        Color3 color;
                        std::getline(fin, line);
                        std::sscanf(line.c_str(), "id=%i", &id);
                        std::getline(fin, line);
                        std::sscanf(
                            line.c_str(),
                            "data=%f,%f,%f",
                            color.data,
                            color.data + 1,
                            color.data + 2);
                        color_nodes_.insert(std::make_pair(id, color));

                        if (id > current_id_)
                        {
                            current_id_ = id;
                        }
                    }
                }
            }
        }

        imnodes::LoadCurrentEditorStateFromDisk("imnodes.ini");
    }

private:
    int current_id_;

    std::unordered_map<int, float> float_nodes_;
    std::unordered_map<int, Color3> color_nodes_;
};

static SaveLoadEditor editor;
} // namespace

void NodeEditorInitialize() { editor.load(); }

void NodeEditorShow() { editor.show(); }

void NodeEditorShutdown() { editor.save(); }
} // namespace example