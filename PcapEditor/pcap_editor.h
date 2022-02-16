#pragma once

#include <imnodes.h>
#include <imgui.h>
#include <SDL_scancode.h>

#include <algorithm>
#include <list>
#include <string>
#include <concepts>


#include <link.hpp>
#include <node.hpp>
#include <attribute.hpp>

namespace PcapEditor
{   

    class Editor{
    public:
        virtual void NodeEditorInitialize() = 0;
        virtual void NodeEditorShow() = 0;
        virtual void NodeEditorShutdown() = 0;

        virtual ~Editor() = default;
    };

    class PcapEditor : public Editor{
    public:
        PcapEditor(const char* editor_name):name(editor_name){};
        virtual void NodeEditorInitialize();
        virtual void NodeEditorShow();
        virtual void NodeEditorShutdown();
    private:

        std::list<Node *> m_endNodes;
        std::list<Node *> m_nodes;
        std::list<Link> m_links;

        std::vector<Overlay *> m_dataOverlays;

        int m_rightClickedId = -1;
        ImVec2 m_rightClickedCoords;
        using NodeError = std::pair<Node *, std::string>;
        std::optional<Node::NodeError> m_currNodeError;

        bool m_continuousEvaluation = false;

        void eraseLink(u32 id);
        void eraseNodes(const std::vector<int> &ids);
        void processNodes();

        // std::string saveNodes();
        // void loadNodes(const std::string &data)

        std::string name;
        ImNodesEditorContext* context = nullptr;
    };

    
} // namespace pcap