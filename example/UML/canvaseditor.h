#include <imgui.h>
#include <imnodes.h>

class CanvasEditor {

public:

    void init() {

        ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);

        ImNodesIO& io = ImNodes::GetIO();
        io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
        io.MultipleSelectModifier.Modifier = &ImGui::GetIO().KeyCtrl;

        ImNodesStyle& style = ImNodes::GetStyle();
        style.Flags |= ImNodesStyleFlags_GridLinesPrimary | ImNodesStyleFlags_GridSnapping;

        //ImFontAtlas::Build
    }

    void show(ImNodesEditorContext* context) {
        ImNodes::EditorContextSet(context);

        ImNodes::BeginNodeEditor();
         ImNodes::EndNodeEditor();
    }

    void exit(ImNodesEditorContext* context) {
        ImNodes::EditorContextFree(context);
        ImNodes::PopAttributeFlag();
    }


};