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
// [SECTION] object pool implementation

struct ImNodesContext;

extern ImNodesContext* GImNodes;

// [SECTION] internal enums

typedef int ImNodesScope;
typedef int ImNodesAttributeType;
typedef int ImNodesUIState;
typedef int ImNodesClickInteractionType;
typedef int ImNodesLinkCreationType;

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

enum ImNodesUIState_
{
    ImNodesUIState_None = 0,
    ImNodesUIState_LinkStarted = 1 << 0,
    ImNodesUIState_LinkDropped = 1 << 1,
    ImNodesUIState_LinkCreated = 1 << 2
};

enum ImNodesClickInteractionType_
{
    ImNodesClickInteractionType_Node,
    ImNodesClickInteractionType_Link,
    ImNodesClickInteractionType_LinkCreation,
    ImNodesClickInteractionType_Panning,
    ImNodesClickInteractionType_BoxSelection,
    ImNodesClickInteractionType_None
};

enum ImNodesLinkCreationType_
{
    ImNodesLinkCreationType_Standard,
    ImNodesLinkCreationType_FromDetach
};

// [SECTION] internal data structures

// The object T must have the following interface:
//
// struct T
// {
//     T();
//
//     int id;
// };
template<typename T>
struct ImObjectPool
{
    ImVector<T>    Pool;
    ImVector<bool> InUse;
    ImVector<int>  FreeList;
    ImGuiStorage   IdMap;

    ImObjectPool() : Pool(), InUse(), FreeList(), IdMap() {}
};

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

// This struct contains all data needed to draw the node in EndNodeEditor(). We store duplicated
// style and color state, because we can't know whether the node is hovered or selected until all
// other nodes have been submitted. The full style state has to be captured in the draw data,
// because the user can customize these per node.
struct ImNodeDrawData
{
    int Id;
    // The coordinates of the node's upper left corner, relative to the editor canvas' upper left
    // corner. See notes/coordinate_spaces.md for further detail.
    ImVec2 EditorSpacePosition;
    ImRect NodeRectangle;
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
    int Id;
    // TODO: It seems this is used to fetch the node id in a few different places.
    //
    // The parent node id is needed in IsLinkCreated(), and in order to check that the pin the link
    // is being attached to is in a different node.
    int ParentNodeIdx;
    // TODO: this can be removed by computing the pin position directly in EndPinAttribute()
    ImRect AttributeRect;
    // TODO: This is used in ShouldLinkSnapToPin to enforce that the end pin is of a different type.
    // It is also used to compute the position of the pin. We could avoid storing this if we
    // computed the position as soon as possible.
    ImNodesAttributeType Type;
    ImNodesPinShape      Shape;
    ImVec2               Pos; // screen-space coordinates
    int                  Flags;

    // NOTE: core data needed to render the pin:
    // - Shape
    // - Pos
    // - Flags
    // - ColorStyle

    struct
    {
        ImU32 Background, Hovered;
    } ColorStyle;

    ImPinData(const int pin_id)
        : Id(pin_id), ParentNodeIdx(), AttributeRect(), Type(ImNodesAttributeType_None),
          Shape(ImNodesPinShape_CircleFilled), Pos(), Flags(ImNodesAttributeFlags_None),
          ColorStyle()
    {
    }
};

struct ImLinkData
{
    int Id;
    int StartPinIdx, EndPinIdx;

    struct
    {
        ImU32 Base, Hovered, Selected;
    } ColorStyle;

    ImLinkData(const int link_id) : Id(link_id), StartPinIdx(), EndPinIdx(), ColorStyle() {}
};

struct ImClickInteractionState
{
    ImNodesClickInteractionType Type;

    struct
    {
        int                     StartPinIdx;
        ImOptionalIndex         EndPinIdx;
        ImNodesLinkCreationType Type;
    } LinkCreation;

    struct
    {
        ImRect Rect;
    } BoxSelector;

    ImClickInteractionState() : Type(ImNodesClickInteractionType_None) {}
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

// [SECTION] editor context and global context structs

struct ImNodesEditorContext
{
    // Contains <node id, node origin> pairs. The node origin is the upper-left corner of the node,
    // and is stored relative to the editor grid. See notes/coordinate_spaces.md for more
    // information. Node origins have to be retained between frames, so that the user doesn't have
    // to manage node position state.
    //
    // TODO: get rid of std::map.
    std::map<int, ImVec2> GridSpaceNodeOrigins;

    ImObjectPool<ImPinData>  Pins;
    ImObjectPool<ImLinkData> Links;

    ImVector<int> NodeDepthOrder;

    // ui related fields

    // TODO: this could be named & documented more specifically
    ImVec2 Panning;

    // TODO: These probably don't need to be retained between frames. These get reset to zero every
    // time we update them.
    ImVector<int> SelectedNodeIndices;
    ImVector<int> SelectedLinkIndices;

    ImClickInteractionState ClickInteraction;

    ImNodesEditorContext()
        : GridSpaceNodeOrigins(), Pins(), Links(), Panning(0.f, 0.f), SelectedNodeIndices(),
          SelectedLinkIndices(), ClickInteraction()
    {
    }
};

struct ImNodesContext
{
    ImNodesEditorContext* DefaultEditorCtx;
    ImNodesEditorContext* EditorCtx;

    // Canvas draw list and helper state
    ImDrawList*   CanvasDrawList;
    ImGuiStorage  NodeIdxToSubmissionIdx;
    ImVector<int> NodeIdxSubmissionOrder;
    ImVector<int> NodeIndicesOverlappingWithMouse;
    ImVector<int> OccludedPinIndices;

    // Canvas extents
    ImVec2 CanvasOriginScreenSpace;
    ImRect CanvasRectScreenSpace;

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

    // Editor state -- nodes, pins, links, and their momentary interaction state, i.e., which
    // elements are being hovered or selected, and which events occurred.

    ImVector<ImNodeDrawData> Nodes;

    // The ids of nodes selected by the user. Selection can happen through clicking on a node, or by
    // dragging a selection box.
    ImVector<int> SelectedNodeIds;

    int CurrentNodeIdx;
    int CurrentPinIdx;
    int CurrentAttributeId;

    ImOptionalIndex HoveredNodeIdx;
    ImOptionalIndex InteractiveNodeIdx;
    ImOptionalIndex HoveredLinkIdx;
    ImOptionalIndex HoveredPinIdx;
    int             HoveredPinFlags;

    ImOptionalIndex DeletedLinkIdx;
    ImOptionalIndex SnapLinkIdx;

    // Event helper state
    // TODO: this should be a part of a state machine, and not a member of the global struct.
    // Unclear what parts of the code this relates to.
    int ImNodesUIState;

    int  ActiveAttributeId;
    bool ActiveAttribute;

    // ImGui::IO cache

    ImVec2 MousePos;

    bool LeftMouseClicked;
    bool LeftMouseReleased;
    bool AltMouseClicked;
    bool LeftMouseDragging;
    bool AltMouseDragging;
};

namespace ImNodes
{
static inline ImNodesEditorContext& EditorContextGet()
{
    // No editor context was set! Did you forget to call ImNodes::CreateContext()?
    assert(GImNodes->EditorCtx != NULL);
    return *GImNodes->EditorCtx;
}

// [SECTION] ObjectPool implementation

template<typename T>
static inline int ObjectPoolFind(ImObjectPool<T>& objects, const int id)
{
    const int index = objects.IdMap.GetInt(static_cast<ImGuiID>(id), -1);
    return index;
}

template<typename T>
static inline void ObjectPoolUpdate(ImObjectPool<T>& objects)
{
    objects.FreeList.clear();
    for (int i = 0; i < objects.InUse.size(); ++i)
    {
        if (!objects.InUse[i])
        {
            objects.IdMap.SetInt(objects.Pool[i].Id, -1);
            objects.FreeList.push_back(i);
            (objects.Pool.Data + i)->~T();
        }
    }
}

template<typename T>
static inline void ObjectPoolReset(ImObjectPool<T>& objects)
{
    if (!objects.InUse.empty())
    {
        memset(objects.InUse.Data, 0, objects.InUse.size_in_bytes());
    }
}

template<typename T>
static inline int ObjectPoolFindOrCreateIndex(ImObjectPool<T>& objects, const int id)
{
    int index = objects.IdMap.GetInt(static_cast<ImGuiID>(id), -1);

    // Construct new object
    if (index == -1)
    {
        if (objects.FreeList.empty())
        {
            index = objects.Pool.size();
            IM_ASSERT(objects.Pool.size() == objects.InUse.size());
            const int new_size = objects.Pool.size() + 1;
            objects.Pool.resize(new_size);
            objects.InUse.resize(new_size);
        }
        else
        {
            index = objects.FreeList.back();
            objects.FreeList.pop_back();
        }
        IM_PLACEMENT_NEW(objects.Pool.Data + index) T(id);
        objects.IdMap.SetInt(static_cast<ImGuiID>(id), index);
    }

    // Flag it as used
    objects.InUse[index] = true;

    return index;
}

template<typename T>
static inline T& ObjectPoolFindOrCreateObject(ImObjectPool<T>& objects, const int id)
{
    const int index = ObjectPoolFindOrCreateIndex(objects, id);
    return objects.Pool[index];
}
} // namespace ImNodes
