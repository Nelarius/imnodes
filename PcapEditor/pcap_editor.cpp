#include "pcap_editor.h"
#include <provider.hpp>
#include <concrete_nodes.hpp>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#undef IMGUI_DEFINE_MATH_OPERATOR
namespace PcapEditor{
    

void PcapEditor::NodeEditorInitialize()
{
    this->context = ImNodes::EditorContextCreate();
    ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);

    ImNodesIO& io = ImNodes::GetIO();
    io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
    
    registerNodes();  
    registerProvider(); 
    
}
void PcapEditor::eraseLink(u32 id) {
        auto link = std::find_if(this->m_links.begin(), this->m_links.end(), [&id](auto link) { return link.getId() == id; });

        if (link == this->m_links.end())
            return;

        for (auto &node : this->m_nodes) {
            for (auto &attribute : node->getAttributes()) {
                attribute.removeConnectedAttribute(id);
            }
        }

        this->m_links.erase(link);

    }

    void PcapEditor::eraseNodes(const std::vector<int> &ids) {
        for (const int id : ids) {
            auto node = std::find_if(this->m_nodes.begin(), this->m_nodes.end(), [&id](auto node) { return node->getId() == id; });

            for (auto &attr : (*node)->getAttributes()) {
                std::vector<u32> linksToRemove;
                for (auto &[linkId, connectedAttr] : attr.getConnectedAttributes())
                    linksToRemove.push_back(linkId);

                for (auto linkId : linksToRemove)
                    eraseLink(linkId);
            }
        }

        for (const int id : ids) {
            auto node = std::find_if(this->m_nodes.begin(), this->m_nodes.end(), [&id](auto node) { return node->getId() == id; });

            std::erase_if(this->m_endNodes, [&id](auto node) { return node->getId() == id; });

            delete *node;

            this->m_nodes.erase(node);
        }

    }
void PcapEditor::NodeEditorShow()
{
    if (ImGui::Begin("hex.builtin.view.data_processor.name", NULL, ImGuiWindowFlags_NoCollapse)) {

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
                ImNodes::ClearNodeSelection();
                ImNodes::ClearLinkSelection();

                this->m_rightClickedCoords = ImGui::GetMousePos();

                if (ImNodes::IsNodeHovered(&this->m_rightClickedId))
                    ImGui::OpenPopup("Node Menu");
                else if (ImNodes::IsLinkHovered(&this->m_rightClickedId))
                    ImGui::OpenPopup("Link Menu");
                else
                    ImGui::OpenPopup("Context Menu");
            }

            if (ImGui::BeginPopup("Context Menu")) {
                Node *node = nullptr;

                if (ImNodes::NumSelectedNodes() > 0 || ImNodes::NumSelectedLinks() > 0) {
                    if (ImGui::MenuItem("hex.builtin.view.data_processor.name")) {
                        std::vector<int> ids;
                        ids.resize(ImNodes::NumSelectedNodes());
                        ImNodes::GetSelectedNodes(ids.data());

                        this->eraseNodes(ids);
                        ImNodes::ClearNodeSelection();

                        ids.resize(ImNodes::NumSelectedLinks());
                        ImNodes::GetSelectedLinks(ids.data());

                        for (auto id : ids)
                            this->eraseLink(id);
                        ImNodes::ClearLinkSelection();
                    }
                }

                for (const auto &[unlocalizedCategory, unlocalizedName, function] : utility::getEntries()) {
                    if (unlocalizedCategory.empty() && unlocalizedName.empty()) {
                        ImGui::Separator();
                    } else if (unlocalizedCategory.empty()) {
                        if (ImGui::MenuItem((unlocalizedName.c_str()))) {
                            node = function();
                        }
                    } else {
                        if (ImGui::BeginMenu((unlocalizedCategory.c_str()))) {
                            if (ImGui::MenuItem((unlocalizedName.c_str()))) {
                                node = function();
                            }
                            ImGui::EndMenu();
                        }
                    }
                }

                if (node != nullptr) {
                    this->m_nodes.push_back(node);

                    bool hasOutput = false;
                    bool hasInput  = false;
                    for (auto &attr : node->getAttributes()) {
                        if (attr.getIOType() == Attribute::IOType::Out)
                            hasOutput = true;

                        if (attr.getIOType() == Attribute::IOType::In)
                            hasInput = true;
                    }

                    if (hasInput && !hasOutput)
                        this->m_endNodes.push_back(node);

                    ImNodes::SetNodeScreenSpacePos(node->getId(), this->m_rightClickedCoords);
                }

                ImGui::EndPopup();
            }

            if (ImGui::BeginPopup("Node Menu")) {
                if (ImGui::MenuItem("hex.builtin.view.data_processor.menu.remove_node"))
                    this->eraseNodes({ this->m_rightClickedId });

                ImGui::EndPopup();
            }

            if (ImGui::BeginPopup("Link Menu")) {
                if (ImGui::MenuItem("hex.builtin.view.data_processor.menu.remove_link"))
                    this->eraseLink(this->m_rightClickedId);

                ImGui::EndPopup();
            }

            {
                int nodeId;
                if (ImNodes::IsNodeHovered(&nodeId) && this->m_currNodeError.has_value() && this->m_currNodeError->first->getId() == nodeId) {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted("hex.builtin.common.error");
                    ImGui::Separator();
                    ImGui::TextUnformatted(this->m_currNodeError->second.c_str());
                    ImGui::EndTooltip();
                }
            }

            if (ImGui::BeginChild("##node_editor", ImGui::GetContentRegionAvail() - ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 1.3))) {
                ImNodes::BeginNodeEditor();

                for (auto &node : this->m_nodes) {
                    const bool hasError = this->m_currNodeError.has_value() && this->m_currNodeError->first == node;

                    if (hasError)
                        ImNodes::PushColorStyle(ImNodesCol_NodeOutline, 0xFF0000FF);

                    ImNodes::BeginNode(node->getId());

                    ImNodes::BeginNodeTitleBar();
                    ImGui::TextUnformatted((node->getUnlocalizedTitle().c_str()));
                    ImNodes::EndNodeTitleBar();

                    node->drawNode();

                    for (auto &attribute : node->getAttributes()) {
                        ImNodesPinShape pinShape;

                        switch (attribute.getType()) {
                            case Attribute::Type::Integer:
                                pinShape = ImNodesPinShape_Circle;
                                break;
                            case Attribute::Type::Float:
                                pinShape = ImNodesPinShape_Triangle;
                                break;
                            case Attribute::Type::Buffer:
                                pinShape = ImNodesPinShape_Quad;
                                break;
                            // case Attribute::Type::Filter:
                            //     pinShape = ImNodesPinShape_CircleFilled;
                            //     break;
                            // case Attribute::Type::Stat:
                            //     pinShape = ImNodesPinShape_TriangleFilled;
                            //     break;
                            case Attribute::Type::String:
                                pinShape = ImNodesPinShape_CircleFilled;
                                break;
                            case Attribute::Type::Pointer:
                                pinShape = ImNodesPinShape_TriangleFilled;
                                break;
                            
                        }

                        if (attribute.getIOType() == Attribute::IOType::In) {
                            ImNodes::BeginInputAttribute(attribute.getId(), pinShape);
                            ImGui::TextUnformatted((attribute.getUnlocalizedName().c_str()));
                            ImNodes::EndInputAttribute();
                        } else if (attribute.getIOType() == Attribute::IOType::Out) {
                            ImNodes::BeginOutputAttribute(attribute.getId(), ImNodesPinShape(pinShape + 1));
                            ImGui::TextUnformatted((attribute.getUnlocalizedName().c_str()));
                            ImNodes::EndOutputAttribute();
                        }
                    }

                    ImNodes::EndNode();

                    if (hasError)
                        ImNodes::PopColorStyle();
                }

                for (const auto &link : this->m_links)
                    ImNodes::Link(link.getId(), link.getFromId(), link.getToId());

                ImNodes::MiniMap(0.2F, ImNodesMiniMapLocation_BottomRight);

                ImNodes::EndNodeEditor();
            }
            ImGui::EndChild();

            // std::printf("process Nodes below");
            if (ImGui::Button("Process") || this->m_continuousEvaluation){                    
                this->processNodes();
            }
                

            ImGui::SameLine();
            ImGui::Checkbox("Continuous evaluation", &this->m_continuousEvaluation);

            {
                int linkId;
                if (ImNodes::IsLinkDestroyed(&linkId)) {
                    this->eraseLink(linkId);
                }
            }

            {
                int from, to;
                if (ImNodes::IsLinkCreated(&from, &to)) {

                    do {
                        Attribute *fromAttr, *toAttr;
                        for (auto &node : this->m_nodes) {
                            for (auto &attribute : node->getAttributes()) {
                                if (attribute.getId() == from)
                                    fromAttr = &attribute;
                                else if (attribute.getId() == to)
                                    toAttr = &attribute;
                            }
                        }

                        if (fromAttr == nullptr || toAttr == nullptr)
                            break;

                        if (fromAttr->getType() != toAttr->getType())
                            break;

                        if (fromAttr->getIOType() == toAttr->getIOType())
                            break;

                        if (!toAttr->getConnectedAttributes().empty())
                            break;

                        auto newLink = this->m_links.emplace_back(from, to);

                        fromAttr->addConnectedAttribute(newLink.getId(), toAttr);
                        toAttr->addConnectedAttribute(newLink.getId(), fromAttr);
                    } while (false);
                }
            }

            {
                const int selectedLinkCount = ImNodes::NumSelectedLinks();
                if (selectedLinkCount > 0 && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete))) {
                    static std::vector<int> selectedLinks;
                    selectedLinks.resize(static_cast<size_t>(selectedLinkCount));
                    ImNodes::GetSelectedLinks(selectedLinks.data());

                    for (const int id : selectedLinks) {
                        eraseLink(id);
                    }
                }
            }

            {
                const int selectedNodeCount = ImNodes::NumSelectedNodes();
                if (selectedNodeCount > 0 && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete))) {
                    static std::vector<int> selectedNodes;
                    selectedNodes.resize(static_cast<size_t>(selectedNodeCount));
                    ImNodes::GetSelectedNodes(selectedNodes.data());

                    this->eraseNodes(selectedNodes);
                }
            }
        }
        ImGui::End();
}
void PcapEditor::processNodes() {
        // std::printf("process enter!");
        if (this->m_dataOverlays.size() != this->m_endNodes.size()) {
            for (auto overlay : this->m_dataOverlays)
                get()->deleteOverlay(overlay);
                
            this->m_dataOverlays.clear();

            for (u32 i = 0; i < this->m_endNodes.size(); i++)
                this->m_dataOverlays.push_back(get()->newOverlay());
                


            u32 overlayIndex = 0;
            for (auto endNode : this->m_endNodes) {
                endNode->setCurrentOverlay(this->m_dataOverlays[overlayIndex]);
                overlayIndex++;
            }
        }

        this->m_currNodeError.reset();
        // std::printf("process set endNote!");

        try {
            for (auto &endNode : this->m_endNodes) {
                endNode->resetOutputData();

                for (auto &node : this->m_nodes)
                    node->resetProcessedInputs();

                endNode->process();
                
            }
        } catch (Node::NodeError &e) {
            this->m_currNodeError = e;

            for (auto overlay : this->m_dataOverlays)
                get()->deleteOverlay(overlay);
                
            this->m_dataOverlays.clear();

        } catch (std::runtime_error &e) {
            std::printf("Node implementation bug! %s\n", e.what());
        } catch(...)
        {
            std::cout<<"*******unknown error occurs!!!!********"<<std::endl;
        }
    }

void PcapEditor::NodeEditorShutdown()
{
    ImNodes::PopAttributeFlag();
    ImNodes::EditorContextFree(this->context);
}
}

