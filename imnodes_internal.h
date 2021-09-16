#pragma once

#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include <assert.h>
#include <limits.h>

#include <map>

// the structure of this file:
//
// [SECTION] internal enums
// [SECTION] internal data structures
// [SECTION] global and editor context structs
// [SECTION] api helpers

struct ImNodesContext;

extern ImNodesContext* GImNodes;

// [SECTION] internal enums

typedef int ImNodesScope;
typedef int ImNodesAttributeType;
typedef int ImNodesUIState;
typedef int ImNodesClickInteractionType;
typedef int ImNodesLinkCreatedFrom;
typedef int ImNodesUIEventType;

enum ImNodesScope_
{
    ImNodesScope_None = 1,
    ImNodesScope_Editor = 1 << 1,
    ImNodesScope_Node = 1 << 2,
    ImNodesScope_Attribute = 1 << 3
};

enum ImNodesAttributeType_
{
    ImNodesAttributeType_None,
    ImNodesAttributeType_Input,
    ImNodesAttributeType_Output
};

enum ImNodesClickInteractionType_
{
    ImNodesClickInteractionType_Node,
    ImNodesClickInteractionType_Link,
    ImNodesClickInteractionType_UnconnectedLink,
    ImNodesClickInteractionType_SnappedLink,
    ImNodesClickInteractionType_Panning,
    ImNodesClickInteractionType_BoxSelection,
    ImNodesClickInteractionType_MiniMapPanning,
    ImNodesClickInteractionType_MiniMapZooming,
    ImNodesClickInteractionType_MiniMapSnapping,
    ImNodesClickInteractionType_ImGuiItem,
    ImNodesClickInteractionType_None
};

enum ImNodesLinkCreatedFrom_
{
    ImNodesLinkCreatedFrom_None,
    ImNodesLinkCreatedFrom_Pin,
    ImNodesLinkCreatedFrom_Detach
};

enum ImNodesUIEventType_
{
    ImNodesUIEventType_None = 0,
    ImNodesUIEventType_LinkStarted = 1 << 0,
    ImNodesUIEventType_LinkDropped = 1 << 1,
    ImNodesUIEventType_LinkCreated = 1 << 2
};

// Callback type used to specify special behavior when hovering a node in the minimap
typedef void (*ImNodesMiniMapNodeHoveringCallback)(int, void*);

// [SECTION] internal data structures

// Emulates std::optional<int> using the sentinel value `INVALID_INDEX`.
struct ImOptionalIndex
{
    ImOptionalIndex() : _Index(INVALID_INDEX) {}
    ImOptionalIndex(const int value) : _Index(value) {}

    // Observers

    inline bool HasValue() const { return _Index != INVALID_INDEX; }

    inline int Value() const
    {
        assert(HasValue());
        return _Index;
    }

    // Modifiers

    inline ImOptionalIndex& operator=(const int value)
    {
        _Index = value;
        return *this;
    }

    inline void Reset() { _Index = INVALID_INDEX; }

    inline bool operator==(const ImOptionalIndex& rhs) const { return _Index == rhs._Index; }

    inline bool operator==(const int rhs) const { return _Index == rhs; }

    inline bool operator!=(const ImOptionalIndex& rhs) const { return _Index != rhs._Index; }

    inline bool operator!=(const int rhs) const { return _Index != rhs; }

    static const int INVALID_INDEX = -1;

private:
    int _Index;
};

// This struct contains all data needed to draw the node in EndNodeEditor(). We duplicate
// node-specific style and color state here, as we can't know whether the node is hovered or
// selected until all the other nodes have been submitted.
struct ImNodeDrawData
{
    int Id;
    // The coordinates of the node's upper left corner, relative to the editor canvas' upper left
    // corner.
    ImVec2 CanvasSpacePosition;
    ImRect BaseRectangle;
    ImRect TitleRectangle;

    struct
    {
        ImU32 Background, BackgroundHovered, BackgroundSelected, Outline, Titlebar, TitlebarHovered,
            TitlebarSelected;
    } ColorStyle;

    struct
    {
        float  CornerRounding;
        ImVec2 Padding;
        float  BorderThickness;
    } LayoutStyle;
};

struct ImPinData
{
    int                  Id;
    ImVec2               ScreenSpacePosition;
    int                  ParentNodeIdx; // TODO: remove
    ImNodesAttributeType Type;
    ImNodesPinShape      Shape;
    int                  Flags;

    struct
    {
        ImU32 Background, Hovered;
    } ColorStyle;

    ImPinData(const int pin_id)
        : Id(pin_id), ScreenSpacePosition(), ParentNodeIdx(), Type(ImNodesAttributeType_None),
          Shape(ImNodesPinShape_CircleFilled), Flags(ImNodesAttributeFlags_None), ColorStyle()
    {
    }
};

struct ImLinkData
{
    int Id;
    int StartPinId, EndPinId;

    struct
    {
        ImU32 Base, Hovered, Selected;
    } ColorStyle;

    ImLinkData(const int link_id) : Id(link_id), StartPinId(), EndPinId(), ColorStyle() {}
};

struct ImLinkGeometry
{
    int         LinkId;
    CubicBezier Curve;

    ImLinkGeometry(const int id, const CubicBezier& cubic_bezier) : LinkId(id), Curve(cubic_bezier)
    {
    }
};

struct ImLinkStartedEvent
{
    int StartPinId;
};

struct ImLinkDroppedEvent
{
    int                    StartPinId;
    ImNodesLinkCreatedFrom CreatedFromType;
};

struct ImLinkCreatedEvent
{
    int                    StartPinId, EndPinId;
    ImNodesLinkCreatedFrom CreatedFromType;
};

struct ImNodesUIEvent
{
    ImNodesUIEventType Type;

    union
    {
        ImLinkStartedEvent LinkStarted;
        ImLinkDroppedEvent LinkDropped;
        ImLinkCreatedEvent LinkCreated;
    };

    // Modifiers

    inline void Reset() { Type = ImNodesUIEventType_None; }

    inline void StartLink(const int start_pin_id)
    {
        Type = ImNodesUIEventType_LinkStarted;
        LinkStarted.StartPinId = start_pin_id;
    }

    inline void DropLink(const int start_pin_id, const ImNodesLinkCreatedFrom created_from_type)
    {
        Type = ImNodesUIEventType_LinkDropped;
        LinkDropped.StartPinId = start_pin_id;
        LinkDropped.CreatedFromType = created_from_type;
    }

    inline void CreateLink(
        const int                    start_pin_id,
        const int                    end_pin_id,
        const ImNodesLinkCreatedFrom created_from_type)
    {
        Type = ImNodesUIEventType_LinkCreated;
        LinkCreated.StartPinId = start_pin_id;
        LinkCreated.EndPinId = end_pin_id;
        LinkCreated.CreatedFromType = created_from_type;
    }

    // Accessors

    inline bool IsLinkStarted() const { return (Type & ImNodesUIEventType_LinkStarted) != 0; }
    inline bool IsLinkDropped() const { return (Type & ImNodesUIEventType_LinkDropped) != 0; }
    inline bool IsLinkCreated() const { return (Type & ImNodesUIEventType_LinkCreated) != 0; }
    inline bool IsLinkCreatedFromSnap() const
    {
        return (Type & ImNodesUIEventType_LinkCreated) != 0 &&
               LinkCreated.CreatedFromType == ImNodesLinkCreatedFrom_Detach;
    }
};

struct ImBoxSelector
{
    ImRect Rectangle;

    ImBoxSelector() : Rectangle() {}
};

// A link which is connected to the mouse cursor at the other end
struct ImUnconnectedLink
{
    int                    StartPinId;
    ImNodesLinkCreatedFrom FromType; // TODO: this could be replaced with a bool
};

struct ImSnappedLink
{
    int StartPinId;
    int SnappedPinId;
};

struct ImClickInteractionState
{
    ImNodesClickInteractionType Type;

    // NOTE: these can't be placed in a union, because SnappedLink state gets pushed on top of
    // UnconnectedLink state.
    // TODO: state stack
    ImUnconnectedLink UnconnectedLink;
    ImSnappedLink     SnappedLink;
    ImBoxSelector     BoxSelector;

    inline void StartUnconnectedLink(const int start_pin_id, const ImNodesLinkCreatedFrom from_type)
    {
        assert(Type == ImNodesClickInteractionType_None);
        Type = ImNodesClickInteractionType_UnconnectedLink;
        UnconnectedLink.StartPinId = start_pin_id;
        UnconnectedLink.FromType = from_type;
    }

    inline void SnapUnconnectedLinkToPin(const int snap_pin_id)
    {
        assert(Type == ImNodesClickInteractionType_UnconnectedLink);
        Type = ImNodesClickInteractionType_SnappedLink;
        const int start_pin_id = UnconnectedLink.StartPinId;
        SnappedLink.StartPinId = start_pin_id;
        SnappedLink.SnappedPinId = snap_pin_id;
    }

    inline void UnsnapLinkFromPin()
    {
        assert(Type == ImNodesClickInteractionType_SnappedLink);
        Type = ImNodesClickInteractionType_UnconnectedLink;
    }

    ImClickInteractionState()
        : Type(ImNodesClickInteractionType_None), UnconnectedLink(), SnappedLink(), BoxSelector()
    {
    }
};

struct ImNodesColElement
{
    ImU32      Color;
    ImNodesCol Item;

    ImNodesColElement(const ImU32 c, const ImNodesCol s) : Color(c), Item(s) {}
};

struct ImNodesStyleVarElement
{
    ImNodesStyleVar Item;
    float           Value;

    ImNodesStyleVarElement(const float value, const ImNodesStyleVar variable)
        : Item(variable), Value(value)
    {
    }
};

// [SECTION] global and editor context structs

struct ImNodesEditorContext
{
    // Contains <node id, node origin> pairs. The node origin is the upper-left corner of the node,
    // and is stored relative to the editor grid. See notes/coordinate_spaces.md for more
    // information. Node origins have to be retained between frames, so that the user doesn't have
    // to manage node position state.
    //
    // TODO: get rid of std::map. Replace with ImPool?
    std::map<int, ImVec2> GridSpaceNodeOrigins;

    // ui related fields
    ImVec2 Panning;

    ImVector<int> SelectedNodeIds;
    ImVector<int> SelectedLinkIds;

    ImClickInteractionState ClickInteraction;

    ImNodesEditorContext()
        : GridSpaceNodeOrigins(), Panning(0.f, 0.f), SelectedNodeIds(), SelectedLinkIds(),
          ClickInteraction()
    {
    }
};

struct ImNodesContext
{
    ImNodesEditorContext* DefaultEditorCtx;
    ImNodesEditorContext* EditorCtx;

    // Canvas draw list and helper state
    ImDrawList* CanvasDrawList;

    // Frame state
    ImVector<ImNodeDrawData> Nodes;
    ImVector<ImVector<int> > NodeIdxToPinIndices;
    ImOptionalIndex
        NodeOverlappingCursor; // TODO: this a temporary downgrade in functionality. When nodes
                               // intersect, only the node created last will overlap.

    ImVector<ImPinData> Pins;
    ImVector<ImRect>    PinAttributeRectangles;
    std::map<int, int>  PinIdToPinIdx;

    ImVector<ImLinkData>     Links;
    ImVector<ImLinkGeometry> LinkGeometries;

    // Canvas extents
    ImVec2 CanvasOriginScreenSpace;
    ImRect CanvasRectScreenSpace;

    // MiniMap state
    ImRect                             MiniMapRectScreenSpace;
    ImVec2                             MiniMapRectPanningOffset;
    float                              MiniMapZoom;
    ImNodesMiniMapNodeHoveringCallback MiniMapNodeHoveringCallback;
    void*                              MiniMapNodeHoveringCallbackUserData;

    // Debug helpers
    ImNodesScope CurrentScope;

    // Configuration state
    ImNodesIO                        Io;
    ImNodesStyle                     Style;
    ImVector<ImNodesColElement>      ColorModifierStack;
    ImVector<ImNodesStyleVarElement> StyleModifierStack;
    ImGuiTextBuffer                  TextBuffer;

    int           CurrentAttributeFlags;
    ImVector<int> AttributeFlagStack;

    // UI element state
    int CurrentNodeIdx; // TODO: can this be deleted?
    int CurrentAttributeId;

    ImOptionalIndex HoveredNodeIdx;
    ImOptionalIndex HoveredLinkIdx;
    ImOptionalIndex HoveredPinIdx;

    ImOptionalIndex DeletedLinkIdx;
    ImOptionalIndex SnapLinkIdx;

    // Event helper state

    ImNodesUIEvent UIEvent;

    int  ActiveAttributeId;
    bool ActiveAttribute;

    // ImGui::IO cache

    ImVec2 MousePos;

    bool  LeftMouseClicked;
    bool  LeftMouseReleased;
    bool  AltMouseClicked;
    bool  LeftMouseDragging;
    bool  AltMouseDragging;
    float AltMouseScrollDelta;
};

namespace ImNodes
{
static inline ImNodesEditorContext& EditorContextGet()
{
    // No editor context was set! Did you forget to call ImNodes::CreateContext()?
    assert(GImNodes->EditorCtx != NULL);
    return *GImNodes->EditorCtx;
}

// [SECTION] api helpers

static inline ImVec2 CalculatePanningOffsetToNode(
    const ImVec2& ss_canvas_center,
    const ImVec2& ss_node_center)
{
    return ss_canvas_center - ss_node_center;
}
} // namespace ImNodes
