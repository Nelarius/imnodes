// the structure of this file:
//
// [SECTION] internal data structures
// [SECTION] global struct
// [SECTION] editor context definition
// [SECTION] draw list helper
// [SECTION] ObjectPool implementation
// [SECTION] ui state logic
// [SECTION] render helpers
// [SECTION] API implementation

#include "imnodes.h"

#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

// Check minimum ImGui version
#define MINIMUM_COMPATIBLE_IMGUI_VERSION 17400
#if IMGUI_VERSION_NUM < MINIMUM_COMPATIBLE_IMGUI_VERSION
#error "Minimum ImGui version requirement not met -- please use a newer version!"
#endif

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <new>
#include <stdint.h>
#include <stdio.h> // for fwrite, ssprintf, sscanf
#include <stdlib.h>
#include <string.h> // strlen, strncmp

namespace imnodes
{
namespace
{
enum ScopeFlags
{
    Scope_None = 1,
    Scope_Editor = 1 << 1,
    Scope_Node = 1 << 2,
    Scope_Attribute = 1 << 3
};

enum AttributeType
{
    AttributeType_None,
    AttributeType_Input,
    AttributeType_Output
};

enum ElementStateChange
{
    ElementStateChange_None = 0,
    ElementStateChange_LinkStarted = 1 << 0,
    ElementStateChange_LinkDropped = 1 << 1,
    ElementStateChange_LinkCreated = 1 << 2
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
struct ObjectPool
{
    ImVector<T> Pool;
    ImVector<bool> InUse;
    ImVector<int> FreeList;
    ImGuiStorage IdMap;

    ObjectPool() : Pool(), InUse(), FreeList(), IdMap() {}
};

// Emulates std::optional<int> using the sentinel value `INVALID_INDEX`.
struct OptionalIndex
{
    OptionalIndex() : _Index(INVALID_INDEX) {}
    OptionalIndex(const int value) : _Index(value) {}

    // Observers

    inline bool HasValue() const { return _Index != INVALID_INDEX; }

    inline int Value() const
    {
        assert(HasValue());
        return _Index;
    }

    // Modifiers

    inline OptionalIndex& operator=(const int value)
    {
        _Index = value;
        return *this;
    }

    inline void Reset() { _Index = INVALID_INDEX; }

    inline bool operator==(const OptionalIndex& rhs) const { return _Index == rhs._Index; }

    inline bool operator==(const int rhs) const { return _Index == rhs; }

    inline bool operator!=(const OptionalIndex& rhs) const { return _Index != rhs._Index; }

    inline bool operator!=(const int rhs) const { return _Index != rhs; }

    static const int INVALID_INDEX = -1;

private:
    int _Index;
};

struct NodeData
{
    int Id;
    ImVec2 Origin; // The node origin is in editor space
    ImRect TitleBarContentRect;
    ImRect Rect;

    struct
    {
        ImU32 Background, BackgroundHovered, BackgroundSelected, Outline, Titlebar, TitlebarHovered,
            TitlebarSelected;
    } ColorStyle;

    struct
    {
        float CornerRounding;
        ImVec2 Padding;
        float BorderThickness;
    } LayoutStyle;

    ImVector<int> PinIndices;
    bool Draggable;

    NodeData(const int node_id)
        : Id(node_id), Origin(100.0f, 100.0f), TitleBarContentRect(),
          Rect(ImVec2(0.0f, 0.0f), ImVec2(0.0f, 0.0f)), ColorStyle(), LayoutStyle(), PinIndices(),
          Draggable(true)
    {
    }

    ~NodeData() { Id = INT_MIN; }
};

struct PinData
{
    int Id;
    int ParentNodeIdx;
    ImRect AttributeRect;
    AttributeType Type;
    PinShape Shape;
    ImVec2 Pos; // screen-space coordinates
    int Flags;

    struct
    {
        ImU32 Background, Hovered;
    } ColorStyle;

    PinData(const int pin_id)
        : Id(pin_id), ParentNodeIdx(), AttributeRect(), Type(AttributeType_None),
          Shape(PinShape_CircleFilled), Pos(), Flags(AttributeFlags_None), ColorStyle()
    {
    }
};

struct LinkData
{
    int Id;
    int StartPinIdx, EndPinIdx;

    struct
    {
        ImU32 Base, Hovered, Selected;
    } ColorStyle;

    LinkData(const int link_id) : Id(link_id), StartPinIdx(), EndPinIdx(), ColorStyle() {}
};

struct LinkPredicate
{
    bool operator()(const LinkData& lhs, const LinkData& rhs) const
    {
        // Do a unique compare by sorting the pins' addresses.
        // This catches duplicate links, whether they are in the
        // same direction or not.
        // Sorting by pin index should have the uniqueness guarantees as sorting
        // by id -- each unique id will get one slot in the link pool array.

        int lhs_start = lhs.StartPinIdx;
        int lhs_end = lhs.EndPinIdx;
        int rhs_start = rhs.StartPinIdx;
        int rhs_end = rhs.EndPinIdx;

        if (lhs_start > lhs_end)
        {
            ImSwap(lhs_start, lhs_end);
        }

        if (rhs_start > rhs_end)
        {
            ImSwap(rhs_start, rhs_end);
        }

        return lhs_start == rhs_start && lhs_end == rhs_end;
    }
};

struct BezierCurve
{
    // the curve control points
    ImVec2 P0, P1, P2, P3;
};

struct LinkBezierData
{
    BezierCurve Bezier;
    int NumSegments;
};

enum ClickInteractionType
{
    ClickInteractionType_Node,
    ClickInteractionType_Link,
    ClickInteractionType_LinkCreation,
    ClickInteractionType_Panning,
    ClickInteractionType_BoxSelection,
    ClickInteractionType_None
};

enum LinkCreationType
{
    LinkCreationType_Standard,
    LinkCreationType_FromDetach
};

struct ClickInteractionState
{
    ClickInteractionType Type;

    struct
    {
        int StartPinIdx;
        OptionalIndex EndPinIdx;
        LinkCreationType Type;
    } LinkCreation;

    struct
    {
        ImRect Rect;
    } BoxSelector;

    ClickInteractionState() : Type(ClickInteractionType_None) {}
};

struct ColorStyleElement
{
    ImU32 Color;
    ColorStyle Item;

    ColorStyleElement(const ImU32 c, const ColorStyle s) : Color(c), Item(s) {}
};

struct StyleElement
{
    StyleVar Item;
    float Value;

    StyleElement(const float value, const StyleVar variable) : Item(variable), Value(value) {}
};
} // namespace
// [SECTION] global struct
// this stores data which only lives for one frame
struct Context
{
    EditorContext* DefaultEditorCtx;
    EditorContext* EditorCtx;

    // Canvas draw list and helper state
    ImDrawList* CanvasDrawList;
    ImGuiStorage NodeIdxToSubmissionIdx;
    ImVector<int> NodeIdxSubmissionOrder;
    ImVector<int> NodeIndicesOverlappingWithMouse;
    ImVector<int> OccludedPinIndices;

    // Canvas extents
    ImVec2 CanvasOriginScreenSpace;
    ImRect CanvasRectScreenSpace;

    // Debug helpers
    ScopeFlags CurrentScope;

    // Configuration state
    IO Io;
    Style Style;
    ImVector<ColorStyleElement> ColorModifierStack;
    ImVector<StyleElement> StyleModifierStack;
    ImGuiTextBuffer TextBuffer;

    int CurrentAttributeFlags;
    ImVector<int> AttributeFlagStack;

    // UI element state
    int CurrentNodeIdx;
    int CurrentPinIdx;
    int CurrentAttributeId;

    OptionalIndex HoveredNodeIdx;
    OptionalIndex InteractiveNodeIdx;
    OptionalIndex HoveredLinkIdx;
    OptionalIndex HoveredPinIdx;
    int HoveredPinFlags;

    OptionalIndex DeletedLinkIdx;
    OptionalIndex SnapLinkIdx;

    // Event helper state
    int ElementStateChange;

    int ActiveAttributeId;
    bool ActiveAttribute;

    // ImGui::IO cache

    ImVec2 MousePos;

    bool LeftMouseClicked;
    bool LeftMouseReleased;
    bool AltMouseClicked;
    bool LeftMouseDragging;
    bool AltMouseDragging;
};

Context* g = NULL;

namespace
{

EditorContext& EditorContextGet()
{
    // No editor context was set! Did you forget to call imnodes::Initialize?
    assert(g->EditorCtx != NULL);
    return *g->EditorCtx;
}

inline ImVec2 EvalBezier(float t, const BezierCurve& bezier)
{
    // B(t) = (1-t)**3 p0 + 3(1 - t)**2 t P1 + 3(1-t)t**2 P2 + t**3 P3
    return ImVec2(
        (1 - t) * (1 - t) * (1 - t) * bezier.P0.x + 3 * (1 - t) * (1 - t) * t * bezier.P1.x +
            3 * (1 - t) * t * t * bezier.P2.x + t * t * t * bezier.P3.x,
        (1 - t) * (1 - t) * (1 - t) * bezier.P0.y + 3 * (1 - t) * (1 - t) * t * bezier.P1.y +
            3 * (1 - t) * t * t * bezier.P2.y + t * t * t * bezier.P3.y);
}

// Calculates the closest point along each bezier curve segment.
ImVec2 GetClosestPointOnCubicBezier(
    const int num_segments,
    const ImVec2& p,
    const BezierCurve& bezier)
{
    IM_ASSERT(num_segments > 0);
    ImVec2 p_last = bezier.P0;
    ImVec2 p_closest;
    float p_closest_dist = FLT_MAX;
    float t_step = 1.0f / (float)num_segments;
    for (int i = 1; i <= num_segments; ++i)
    {
        ImVec2 p_current = EvalBezier(t_step * i, bezier);
        ImVec2 p_line = ImLineClosestPoint(p_last, p_current, p);
        float dist = ImLengthSqr(p - p_line);
        if (dist < p_closest_dist)
        {
            p_closest = p_line;
            p_closest_dist = dist;
        }
        p_last = p_current;
    }
    return p_closest;
}

inline float GetDistanceToCubicBezier(
    const ImVec2& pos,
    const BezierCurve& bezier,
    const int num_segments)
{
    const ImVec2 point_on_curve = GetClosestPointOnCubicBezier(num_segments, pos, bezier);

    const ImVec2 to_curve = point_on_curve - pos;
    return ImSqrt(ImLengthSqr(to_curve));
}

inline ImRect GetContainingRectForBezierCurve(const BezierCurve& bezier)
{
    const ImVec2 min = ImVec2(ImMin(bezier.P0.x, bezier.P3.x), ImMin(bezier.P0.y, bezier.P3.y));
    const ImVec2 max = ImVec2(ImMax(bezier.P0.x, bezier.P3.x), ImMax(bezier.P0.y, bezier.P3.y));

    const float hover_distance = g->Style.LinkHoverDistance;

    ImRect rect(min, max);
    rect.Add(bezier.P1);
    rect.Add(bezier.P2);
    rect.Expand(ImVec2(hover_distance, hover_distance));

    return rect;
}

inline LinkBezierData GetLinkRenderable(
    ImVec2 start,
    ImVec2 end,
    const AttributeType start_type,
    const float line_segments_per_length)
{
    assert((start_type == AttributeType_Input) || (start_type == AttributeType_Output));
    if (start_type == AttributeType_Input)
    {
        ImSwap(start, end);
    }

    const float link_length = ImSqrt(ImLengthSqr(end - start));
    const ImVec2 offset = ImVec2(0.25f * link_length, 0.f);
    LinkBezierData link_data;
    link_data.Bezier.P0 = start;
    link_data.Bezier.P1 = start + offset;
    link_data.Bezier.P2 = end - offset;
    link_data.Bezier.P3 = end;
    link_data.NumSegments = ImMax(static_cast<int>(link_length * line_segments_per_length), 1);
    return link_data;
}

inline float EvalImplicitLineEq(const ImVec2& p1, const ImVec2& p2, const ImVec2& p)
{
    return (p2.y - p1.y) * p.x + (p1.x - p2.x) * p.y + (p2.x * p1.y - p1.x * p2.y);
}

inline int Sign(float val) { return int(val > 0.0f) - int(val < 0.0f); }

inline bool RectangleOverlapsLineSegment(const ImRect& rect, const ImVec2& p1, const ImVec2& p2)
{
    // Trivial case: rectangle contains an endpoint
    if (rect.Contains(p1) || rect.Contains(p2))
    {
        return true;
    }

    // Flip rectangle if necessary
    ImRect flip_rect = rect;

    if (flip_rect.Min.x > flip_rect.Max.x)
    {
        ImSwap(flip_rect.Min.x, flip_rect.Max.x);
    }

    if (flip_rect.Min.y > flip_rect.Max.y)
    {
        ImSwap(flip_rect.Min.y, flip_rect.Max.y);
    }

    // Trivial case: line segment lies to one particular side of rectangle
    if ((p1.x < flip_rect.Min.x && p2.x < flip_rect.Min.x) ||
        (p1.x > flip_rect.Max.x && p2.x > flip_rect.Max.x) ||
        (p1.y < flip_rect.Min.y && p2.y < flip_rect.Min.y) ||
        (p1.y > flip_rect.Max.y && p2.y > flip_rect.Max.y))
    {
        return false;
    }

    const int corner_signs[4] = {
        Sign(EvalImplicitLineEq(p1, p2, flip_rect.Min)),
        Sign(EvalImplicitLineEq(p1, p2, ImVec2(flip_rect.Max.x, flip_rect.Min.y))),
        Sign(EvalImplicitLineEq(p1, p2, ImVec2(flip_rect.Min.x, flip_rect.Max.y))),
        Sign(EvalImplicitLineEq(p1, p2, flip_rect.Max))};

    int sum = 0;
    int sum_abs = 0;

    for (int i = 0; i < 4; ++i)
    {
        sum += corner_signs[i];
        sum_abs += abs(corner_signs[i]);
    }

    // At least one corner of rectangle lies on a different side of line segment
    return abs(sum) != sum_abs;
}

inline bool RectangleOverlapsBezier(const ImRect& rectangle, const LinkBezierData& link_data)
{
    ImVec2 current = EvalBezier(0.f, link_data.Bezier);
    const float dt = 1.0f / link_data.NumSegments;
    for (int s = 0; s < link_data.NumSegments; ++s)
    {
        ImVec2 next = EvalBezier(static_cast<float>((s + 1) * dt), link_data.Bezier);
        if (RectangleOverlapsLineSegment(rectangle, current, next))
        {
            return true;
        }
        current = next;
    }
    return false;
}

inline bool RectangleOverlapsLink(
    const ImRect& rectangle,
    const ImVec2& start,
    const ImVec2& end,
    const AttributeType start_type)
{
    // First level: simple rejection test via rectangle overlap:

    ImRect lrect = ImRect(start, end);
    if (lrect.Min.x > lrect.Max.x)
    {
        ImSwap(lrect.Min.x, lrect.Max.x);
    }

    if (lrect.Min.y > lrect.Max.y)
    {
        ImSwap(lrect.Min.y, lrect.Max.y);
    }

    if (rectangle.Overlaps(lrect))
    {
        // First, check if either one or both endpoinds are trivially contained
        // in the rectangle

        if (rectangle.Contains(start) || rectangle.Contains(end))
        {
            return true;
        }

        // Second level of refinement: do a more expensive test against the
        // link

        const LinkBezierData link_data =
            GetLinkRenderable(start, end, start_type, g->Style.LinkLineSegmentsPerLength);
        return RectangleOverlapsBezier(rectangle, link_data);
    }

    return false;
}
} // namespace

// [SECTION] editor context definition

struct EditorContext
{
    ObjectPool<NodeData> Nodes;
    ObjectPool<PinData> Pins;
    ObjectPool<LinkData> Links;

    ImVector<int> NodeDepthOrder;

    // ui related fields
    ImVec2 Panning;

    ImVector<int> SelectedNodeIndices;
    ImVector<int> SelectedLinkIndices;

    ClickInteractionState ClickInteraction;

    EditorContext()
        : Nodes(), Pins(), Links(), Panning(0.f, 0.f), SelectedNodeIndices(), SelectedLinkIndices(),
          ClickInteraction()
    {
    }
};

namespace
{
// [SECTION] draw list helper

void ImDrawListGrowChannels(ImDrawList* draw_list, const int num_channels)
{
    ImDrawListSplitter& splitter = draw_list->_Splitter;

    if (splitter._Count == 1)
    {
        splitter.Split(draw_list, num_channels + 1);
        return;
    }

    // NOTE: this logic has been lifted from ImDrawListSplitter::Split with slight modifications
    // to allow nested splits. The main modification is that we only create new ImDrawChannel
    // instances after splitter._Count, instead of over the whole splitter._Channels array like
    // the regular ImDrawListSplitter::Split method does.

    const int old_channel_capacity = splitter._Channels.Size;
    // NOTE: _Channels is not resized down, and therefore _Count <= _Channels.size()!
    const int old_channel_count = splitter._Count;
    const int requested_channel_count = old_channel_count + num_channels;
    if (old_channel_capacity < old_channel_count + num_channels)
    {
        splitter._Channels.resize(requested_channel_count);
    }

    splitter._Count = requested_channel_count;

    for (int i = old_channel_count; i < requested_channel_count; ++i)
    {
        ImDrawChannel& channel = splitter._Channels[i];

        // If we're inside the old capacity region of the array, we need to reuse the existing
        // memory of the command and index buffers.
        if (i < old_channel_capacity)
        {
            channel._CmdBuffer.resize(0);
            channel._IdxBuffer.resize(0);
        }
        // Else, we need to construct new draw channels.
        else
        {
            IM_PLACEMENT_NEW(&channel) ImDrawChannel();
        }

        {
            ImDrawCmd draw_cmd;
            draw_cmd.ClipRect = draw_list->_ClipRectStack.back();
            draw_cmd.TextureId = draw_list->_TextureIdStack.back();
            channel._CmdBuffer.push_back(draw_cmd);
        }
    }
}

void ImDrawListSplitterSwapChannels(
    ImDrawListSplitter& splitter,
    const int lhs_idx,
    const int rhs_idx)
{
    if (lhs_idx == rhs_idx)
    {
        return;
    }

    assert(lhs_idx >= 0 && lhs_idx < splitter._Count);
    assert(rhs_idx >= 0 && rhs_idx < splitter._Count);

    ImDrawChannel& lhs_channel = splitter._Channels[lhs_idx];
    ImDrawChannel& rhs_channel = splitter._Channels[rhs_idx];
    lhs_channel._CmdBuffer.swap(rhs_channel._CmdBuffer);
    lhs_channel._IdxBuffer.swap(rhs_channel._IdxBuffer);

    const int current_channel = splitter._Current;

    if (current_channel == lhs_idx)
    {
        splitter._Current = rhs_idx;
    }
    else if (current_channel == rhs_idx)
    {
        splitter._Current = lhs_idx;
    }
}

void DrawListSet(ImDrawList* window_draw_list)
{
    g->CanvasDrawList = window_draw_list;
    g->NodeIdxToSubmissionIdx.Clear();
    g->NodeIdxSubmissionOrder.clear();
}

// The draw list channels are structured as follows. First we have our base channel, the canvas grid
// on which we render the grid lines in BeginNodeEditor(). The base channel is the reason
// draw_list_submission_idx_to_background_channel_idx offsets the index by one. Each BeginNode()
// call appends two new draw channels, for the node background and foreground. The node foreground
// is the channel into which the node's ImGui content is rendered. Finally, in EndNodeEditor() we
// append one last draw channel for rendering the selection box and the incomplete link on top of
// everything else.
//
// +----------+----------+----------+----------+----------+----------+
// |          |          |          |          |          |          |
// |canvas    |node      |node      |...       |...       |click     |
// |grid      |background|foreground|          |          |interaction
// |          |          |          |          |          |          |
// +----------+----------+----------+----------+----------+----------+
//            |                     |
//            |   submission idx    |
//            |                     |
//            -----------------------

void DrawListAddNode(const int node_idx)
{
    g->NodeIdxToSubmissionIdx.SetInt(
        static_cast<ImGuiID>(node_idx), g->NodeIdxSubmissionOrder.Size);
    g->NodeIdxSubmissionOrder.push_back(node_idx);
    ImDrawListGrowChannels(g->CanvasDrawList, 2);
}

void DrawListAppendClickInteractionChannel()
{
    // NOTE: don't use this function outside of EndNodeEditor. Using this before all nodes have been
    // added will screw up the node draw order.
    ImDrawListGrowChannels(g->CanvasDrawList, 1);
}

int DrawListSubmissionIdxToBackgroundChannelIdx(const int submission_idx)
{
    // NOTE: the first channel is the canvas background, i.e. the grid
    return 1 + 2 * submission_idx;
}

int DrawListSubmissionIdxToForegroundChannelIdx(const int submission_idx)
{
    return DrawListSubmissionIdxToBackgroundChannelIdx(submission_idx) + 1;
}

void DrawListActivateClickInteractionChannel()
{
    g->CanvasDrawList->_Splitter.SetCurrentChannel(
        g->CanvasDrawList, g->CanvasDrawList->_Splitter._Count - 1);
}

void DrawListActivateCurrentNodeForeground()
{
    const int foreground_channel_idx =
        DrawListSubmissionIdxToForegroundChannelIdx(g->NodeIdxSubmissionOrder.Size - 1);
    g->CanvasDrawList->_Splitter.SetCurrentChannel(g->CanvasDrawList, foreground_channel_idx);
}

void DrawListActivateNodeBackground(const int node_idx)
{
    const int submission_idx = g->NodeIdxToSubmissionIdx.GetInt(static_cast<ImGuiID>(node_idx), -1);
    // There is a discrepancy in the submitted node count and the rendered node count! Did you call
    // one of the following functions
    // * EditorContextMoveToNode
    // * SetNodeScreenSpacePos
    // * SetNodeGridSpacePos
    // * SetNodeDraggable
    // after the BeginNode/EndNode function calls?
    assert(submission_idx != -1);
    const int background_channel_idx = DrawListSubmissionIdxToBackgroundChannelIdx(submission_idx);
    g->CanvasDrawList->_Splitter.SetCurrentChannel(g->CanvasDrawList, background_channel_idx);
}

void DrawListSwapSubmissionIndices(const int lhs_idx, const int rhs_idx)
{
    assert(lhs_idx != rhs_idx);

    const int lhs_foreground_channel_idx = DrawListSubmissionIdxToForegroundChannelIdx(lhs_idx);
    const int lhs_background_channel_idx = DrawListSubmissionIdxToBackgroundChannelIdx(lhs_idx);
    const int rhs_foreground_channel_idx = DrawListSubmissionIdxToForegroundChannelIdx(rhs_idx);
    const int rhs_background_channel_idx = DrawListSubmissionIdxToBackgroundChannelIdx(rhs_idx);

    ImDrawListSplitterSwapChannels(
        g->CanvasDrawList->_Splitter, lhs_background_channel_idx, rhs_background_channel_idx);
    ImDrawListSplitterSwapChannels(
        g->CanvasDrawList->_Splitter, lhs_foreground_channel_idx, rhs_foreground_channel_idx);
}

void DrawListSortChannelsByDepth(const ImVector<int>& node_idx_depth_order)
{
    if (g->NodeIdxToSubmissionIdx.Data.Size < 2)
    {
        return;
    }

    assert(node_idx_depth_order.Size == g->NodeIdxSubmissionOrder.Size);

    int start_idx = node_idx_depth_order.Size - 1;

    while (node_idx_depth_order[start_idx] == g->NodeIdxSubmissionOrder[start_idx])
    {
        if (--start_idx == 0)
        {
            // early out if submission order and depth order are the same
            return;
        }
    }

    // TODO: this is an O(N^2) algorithm. It might be worthwhile revisiting this to see if the time
    // complexity can be reduced.

    for (int depth_idx = start_idx; depth_idx > 0; --depth_idx)
    {
        const int node_idx = node_idx_depth_order[depth_idx];

        // Find the current index of the node_idx in the submission order array
        int submission_idx = -1;
        for (int i = 0; i < g->NodeIdxSubmissionOrder.Size; ++i)
        {
            if (g->NodeIdxSubmissionOrder[i] == node_idx)
            {
                submission_idx = i;
                break;
            }
        }
        assert(submission_idx >= 0);

        if (submission_idx == depth_idx)
        {
            continue;
        }

        for (int j = submission_idx; j < depth_idx; ++j)
        {
            DrawListSwapSubmissionIndices(j, j + 1);
            ImSwap(g->NodeIdxSubmissionOrder[j], g->NodeIdxSubmissionOrder[j + 1]);
        }
    }
}

// [SECTION] ObjectPool implementation

template<typename T>
int ObjectPoolFind(ObjectPool<T>& objects, const int id)
{
    const int index = objects.IdMap.GetInt(static_cast<ImGuiID>(id), -1);
    return index;
}

template<typename T>
void ObjectPoolUpdate(ObjectPool<T>& objects)
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

template<>
void ObjectPoolUpdate(ObjectPool<NodeData>& nodes)
{
    nodes.FreeList.clear();
    for (int i = 0; i < nodes.InUse.size(); ++i)
    {
        if (nodes.InUse[i])
        {
            nodes.Pool[i].PinIndices.clear();
        }
        else
        {
            const int previous_id = nodes.Pool[i].Id;
            const int previous_idx = nodes.IdMap.GetInt(previous_id, -1);

            if (previous_idx != -1)
            {
                assert(previous_idx == i);
                // Remove node idx form depth stack the first time we detect that this idx slot is
                // unused
                ImVector<int>& depth_stack = EditorContextGet().NodeDepthOrder;
                const int* const elem = depth_stack.find(i);
                assert(elem != depth_stack.end());
                depth_stack.erase(elem);
            }

            nodes.IdMap.SetInt(previous_id, -1);
            nodes.FreeList.push_back(i);
            (nodes.Pool.Data + i)->~NodeData();
        }
    }
}

template<typename T>
void ObjectPoolReset(ObjectPool<T>& objects)
{
    if (!objects.InUse.empty())
    {
        memset(objects.InUse.Data, 0, objects.InUse.size_in_bytes());
    }
}

template<typename T>
int ObjectPoolFindOrCreateIndex(ObjectPool<T>& objects, const int id)
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

template<>
int ObjectPoolFindOrCreateIndex(ObjectPool<NodeData>& nodes, const int node_id)
{
    int node_idx = nodes.IdMap.GetInt(static_cast<ImGuiID>(node_id), -1);

    // Construct new node
    if (node_idx == -1)
    {
        if (nodes.FreeList.empty())
        {
            node_idx = nodes.Pool.size();
            IM_ASSERT(nodes.Pool.size() == nodes.InUse.size());
            const int new_size = nodes.Pool.size() + 1;
            nodes.Pool.resize(new_size);
            nodes.InUse.resize(new_size);
        }
        else
        {
            node_idx = nodes.FreeList.back();
            nodes.FreeList.pop_back();
        }
        IM_PLACEMENT_NEW(nodes.Pool.Data + node_idx) NodeData(node_id);
        nodes.IdMap.SetInt(static_cast<ImGuiID>(node_id), node_idx);

        EditorContext& editor = EditorContextGet();
        editor.NodeDepthOrder.push_back(node_idx);
    }

    // Flag node as used
    nodes.InUse[node_idx] = true;

    return node_idx;
}

template<typename T>
T& ObjectPoolFindOrCreateObject(ObjectPool<T>& objects, const int id)
{
    const int index = ObjectPoolFindOrCreateIndex(objects, id);
    return objects.Pool[index];
}

// [SECTION] ui state logic

ImVec2 GetScreenSpacePinCoordinates(
    const ImRect& node_rect,
    const ImRect& attribute_rect,
    const AttributeType type)
{
    assert(type == AttributeType_Input || type == AttributeType_Output);
    const float x = type == AttributeType_Input ? (node_rect.Min.x - g->Style.PinOffset)
                                                : (node_rect.Max.x + g->Style.PinOffset);
    return ImVec2(x, 0.5f * (attribute_rect.Min.y + attribute_rect.Max.y));
}

ImVec2 GetScreenSpacePinCoordinates(const EditorContext& editor, const PinData& pin)
{
    const ImRect& parent_node_rect = editor.Nodes.Pool[pin.ParentNodeIdx].Rect;
    return GetScreenSpacePinCoordinates(parent_node_rect, pin.AttributeRect, pin.Type);
}

bool MouseInCanvas()
{
    // This flag should be true either when hovering or clicking something in the canvas.
    const bool is_window_hovered_or_focused = ImGui::IsWindowHovered() || ImGui::IsWindowFocused();

    return is_window_hovered_or_focused && g->CanvasRectScreenSpace.Contains(ImGui::GetMousePos());
}

void BeginNodeSelection(EditorContext& editor, const int node_idx)
{
    // Don't start selecting a node if we are e.g. already creating and dragging
    // a new link! New link creation can happen when the mouse is clicked over
    // a node, but within the hover radius of a pin.
    if (editor.ClickInteraction.Type != ClickInteractionType_None)
    {
        return;
    }

    editor.ClickInteraction.Type = ClickInteractionType_Node;
    // If the node is not already contained in the selection, then we want only
    // the interaction node to be selected, effective immediately.
    //
    // Otherwise, we want to allow for the possibility of multiple nodes to be
    // moved at once.
    if (!editor.SelectedNodeIndices.contains(node_idx))
    {
        editor.SelectedNodeIndices.clear();
        editor.SelectedLinkIndices.clear();
        editor.SelectedNodeIndices.push_back(node_idx);

        // Ensure that individually selected nodes get rendered on top
        ImVector<int>& depth_stack = editor.NodeDepthOrder;
        const int* const elem = depth_stack.find(node_idx);
        assert(elem != depth_stack.end());
        depth_stack.erase(elem);
        depth_stack.push_back(node_idx);
    }
}

void BeginLinkSelection(EditorContext& editor, const int link_idx)
{
    editor.ClickInteraction.Type = ClickInteractionType_Link;
    // When a link is selected, clear all other selections, and insert the link
    // as the sole selection.
    editor.SelectedNodeIndices.clear();
    editor.SelectedLinkIndices.clear();
    editor.SelectedLinkIndices.push_back(link_idx);
}

void BeginLinkDetach(EditorContext& editor, const int link_idx, const int detach_pin_idx)
{
    const LinkData& link = editor.Links.Pool[link_idx];
    ClickInteractionState& state = editor.ClickInteraction;
    state.LinkCreation.EndPinIdx.Reset();
    state.LinkCreation.StartPinIdx =
        detach_pin_idx == link.StartPinIdx ? link.EndPinIdx : link.StartPinIdx;
    g->DeletedLinkIdx = link_idx;
}

void BeginLinkInteraction(EditorContext& editor, const int link_idx)
{
    // First check if we are clicking a link in the vicinity of a pin.
    // This may result in a link detach via click and drag.
    if (editor.ClickInteraction.Type == ClickInteractionType_LinkCreation)
    {
        if ((g->HoveredPinFlags & AttributeFlags_EnableLinkDetachWithDragClick) != 0)
        {
            BeginLinkDetach(editor, link_idx, g->HoveredPinIdx.Value());
            editor.ClickInteraction.LinkCreation.Type = LinkCreationType_FromDetach;
        }
    }
    // If we aren't near a pin, check if we are clicking the link with the
    // modifier pressed. This may also result in a link detach via clicking.
    else
    {
        const bool modifier_pressed = g->Io.LinkDetachWithModifierClick.Modifier == NULL
                                          ? false
                                          : *g->Io.LinkDetachWithModifierClick.Modifier;

        if (modifier_pressed)
        {
            const LinkData& link = editor.Links.Pool[link_idx];
            const PinData& start_pin = editor.Pins.Pool[link.StartPinIdx];
            const PinData& end_pin = editor.Pins.Pool[link.EndPinIdx];
            const ImVec2& mouse_pos = g->MousePos;
            const float dist_to_start = ImLengthSqr(start_pin.Pos - mouse_pos);
            const float dist_to_end = ImLengthSqr(end_pin.Pos - mouse_pos);
            const int closest_pin_idx =
                dist_to_start < dist_to_end ? link.StartPinIdx : link.EndPinIdx;

            editor.ClickInteraction.Type = ClickInteractionType_LinkCreation;
            BeginLinkDetach(editor, link_idx, closest_pin_idx);
            editor.ClickInteraction.LinkCreation.Type = LinkCreationType_FromDetach;
        }
        else
        {
            BeginLinkSelection(editor, link_idx);
        }
    }
}

void BeginLinkCreation(EditorContext& editor, const int hovered_pin_idx)
{
    editor.ClickInteraction.Type = ClickInteractionType_LinkCreation;
    editor.ClickInteraction.LinkCreation.StartPinIdx = hovered_pin_idx;
    editor.ClickInteraction.LinkCreation.EndPinIdx.Reset();
    editor.ClickInteraction.LinkCreation.Type = LinkCreationType_Standard;
    g->ElementStateChange |= ElementStateChange_LinkStarted;
}

void BeginCanvasInteraction(EditorContext& editor)
{
    const bool any_ui_element_hovered = g->HoveredNodeIdx.HasValue() ||
                                        g->HoveredLinkIdx.HasValue() ||
                                        g->HoveredPinIdx.HasValue() || ImGui::IsAnyItemHovered();

    const bool mouse_not_in_canvas = !MouseInCanvas();

    if (editor.ClickInteraction.Type != ClickInteractionType_None || any_ui_element_hovered ||
        mouse_not_in_canvas)
    {
        return;
    }

    const bool started_panning = g->AltMouseClicked;

    if (started_panning)
    {
        editor.ClickInteraction.Type = ClickInteractionType_Panning;
    }
    else if (g->LeftMouseClicked)
    {
        editor.ClickInteraction.Type = ClickInteractionType_BoxSelection;
        editor.ClickInteraction.BoxSelector.Rect.Min = g->MousePos;
    }
}

void BoxSelectorUpdateSelection(EditorContext& editor, ImRect box_rect)
{
    // Invert box selector coordinates as needed

    if (box_rect.Min.x > box_rect.Max.x)
    {
        ImSwap(box_rect.Min.x, box_rect.Max.x);
    }

    if (box_rect.Min.y > box_rect.Max.y)
    {
        ImSwap(box_rect.Min.y, box_rect.Max.y);
    }

    // Update node selection

    editor.SelectedNodeIndices.clear();

    // Test for overlap against node rectangles

    for (int node_idx = 0; node_idx < editor.Nodes.Pool.size(); ++node_idx)
    {
        if (editor.Nodes.InUse[node_idx])
        {
            NodeData& node = editor.Nodes.Pool[node_idx];
            if (box_rect.Overlaps(node.Rect))
            {
                editor.SelectedNodeIndices.push_back(node_idx);
            }
        }
    }

    // Update link selection

    editor.SelectedLinkIndices.clear();

    // Test for overlap against links

    for (int link_idx = 0; link_idx < editor.Links.Pool.size(); ++link_idx)
    {
        if (editor.Links.InUse[link_idx])
        {
            const LinkData& link = editor.Links.Pool[link_idx];

            const PinData& pin_start = editor.Pins.Pool[link.StartPinIdx];
            const PinData& pin_end = editor.Pins.Pool[link.EndPinIdx];
            const ImRect& node_start_rect = editor.Nodes.Pool[pin_start.ParentNodeIdx].Rect;
            const ImRect& node_end_rect = editor.Nodes.Pool[pin_end.ParentNodeIdx].Rect;

            const ImVec2 start = GetScreenSpacePinCoordinates(
                node_start_rect, pin_start.AttributeRect, pin_start.Type);
            const ImVec2 end =
                GetScreenSpacePinCoordinates(node_end_rect, pin_end.AttributeRect, pin_end.Type);

            // Test
            if (RectangleOverlapsLink(box_rect, start, end, pin_start.Type))
            {
                editor.SelectedLinkIndices.push_back(link_idx);
            }
        }
    }
}

void TranslateSelectedNodes(EditorContext& editor)
{
    if (g->LeftMouseDragging)
    {
        const ImGuiIO& io = ImGui::GetIO();
        for (int i = 0; i < editor.SelectedNodeIndices.size(); ++i)
        {
            const int node_idx = editor.SelectedNodeIndices[i];
            NodeData& node = editor.Nodes.Pool[node_idx];
            if (node.Draggable)
            {
                node.Origin += io.MouseDelta;
            }
        }
    }
}

OptionalIndex FindDuplicateLink(
    const EditorContext& editor,
    const int start_pin_idx,
    const int end_pin_idx)
{
    LinkData test_link(0);
    test_link.StartPinIdx = start_pin_idx;
    test_link.EndPinIdx = end_pin_idx;
    for (int link_idx = 0; link_idx < editor.Links.Pool.size(); ++link_idx)
    {
        const LinkData& link = editor.Links.Pool[link_idx];
        if (LinkPredicate()(test_link, link) && editor.Links.InUse[link_idx])
        {
            return OptionalIndex(link_idx);
        }
    }

    return OptionalIndex();
}

bool ShouldLinkSnapToPin(
    const EditorContext& editor,
    const PinData& start_pin,
    const int hovered_pin_idx,
    const OptionalIndex duplicate_link)
{
    const PinData& end_pin = editor.Pins.Pool[hovered_pin_idx];

    // The end pin must be in a different node
    if (start_pin.ParentNodeIdx == end_pin.ParentNodeIdx)
    {
        return false;
    }

    // The end pin must be of a different type
    if (start_pin.Type == end_pin.Type)
    {
        return false;
    }

    // The link to be created must not be a duplicate, unless it is the link which was created on
    // snap. In that case we want to snap, since we want it to appear visually as if the created
    // link remains snapped to the pin.
    if (duplicate_link.HasValue() && !(duplicate_link == g->SnapLinkIdx))
    {
        return false;
    }

    return true;
}

void ClickInteractionUpdate(EditorContext& editor)
{
    switch (editor.ClickInteraction.Type)
    {
    case ClickInteractionType_BoxSelection:
    {
        ImRect& box_rect = editor.ClickInteraction.BoxSelector.Rect;
        box_rect.Max = g->MousePos;

        BoxSelectorUpdateSelection(editor, box_rect);

        const ImU32 box_selector_color = g->Style.Colors[ColorStyle_BoxSelector];
        const ImU32 box_selector_outline = g->Style.Colors[ColorStyle_BoxSelectorOutline];
        g->CanvasDrawList->AddRectFilled(box_rect.Min, box_rect.Max, box_selector_color);
        g->CanvasDrawList->AddRect(box_rect.Min, box_rect.Max, box_selector_outline);

        if (g->LeftMouseReleased)
        {
            ImVector<int>& depth_stack = editor.NodeDepthOrder;
            const ImVector<int>& selected_idxs = editor.SelectedNodeIndices;

            // Bump the selected node indices, in order, to the top of the depth stack.
            // NOTE: this algorithm has worst case time complexity of O(N^2), if the node selection
            // is ~ N (due to selected_idxs.contains()).

            if ((selected_idxs.Size > 0) && (selected_idxs.Size < depth_stack.Size))
            {
                int num_moved = 0; // The number of indices moved. Stop after selected_idxs.Size
                for (int i = 0; i < depth_stack.Size - selected_idxs.Size; ++i)
                {
                    for (int node_idx = depth_stack[i]; selected_idxs.contains(node_idx);
                         node_idx = depth_stack[i])
                    {
                        depth_stack.erase(depth_stack.begin() + static_cast<size_t>(i));
                        depth_stack.push_back(node_idx);
                        ++num_moved;
                    }

                    if (num_moved == selected_idxs.Size)
                    {
                        break;
                    }
                }
            }

            editor.ClickInteraction.Type = ClickInteractionType_None;
        }
    }
    break;
    case ClickInteractionType_Node:
    {
        TranslateSelectedNodes(editor);

        if (g->LeftMouseReleased)
        {
            editor.ClickInteraction.Type = ClickInteractionType_None;
        }
    }
    break;
    case ClickInteractionType_Link:
    {
        if (g->LeftMouseReleased)
        {
            editor.ClickInteraction.Type = ClickInteractionType_None;
        }
    }
    break;
    case ClickInteractionType_LinkCreation:
    {
        const PinData& start_pin =
            editor.Pins.Pool[editor.ClickInteraction.LinkCreation.StartPinIdx];

        const OptionalIndex maybe_duplicate_link_idx =
            g->HoveredPinIdx.HasValue() ? FindDuplicateLink(
                                              editor,
                                              editor.ClickInteraction.LinkCreation.StartPinIdx,
                                              g->HoveredPinIdx.Value())
                                        : OptionalIndex();

        const bool should_snap =
            g->HoveredPinIdx.HasValue() &&
            ShouldLinkSnapToPin(
                editor, start_pin, g->HoveredPinIdx.Value(), maybe_duplicate_link_idx);

        // If we created on snap and the hovered pin is empty or changed, then we need signal that
        // the link's state has changed.
        const bool snapping_pin_changed =
            editor.ClickInteraction.LinkCreation.EndPinIdx.HasValue() &&
            !(g->HoveredPinIdx == editor.ClickInteraction.LinkCreation.EndPinIdx);

        // Detach the link that was created by this link event if it's no longer in snap range
        if (snapping_pin_changed && g->SnapLinkIdx.HasValue())
        {
            BeginLinkDetach(
                editor,
                g->SnapLinkIdx.Value(),
                editor.ClickInteraction.LinkCreation.EndPinIdx.Value());
        }

        const ImVec2 start_pos = GetScreenSpacePinCoordinates(editor, start_pin);
        // If we are within the hover radius of a receiving pin, snap the link
        // endpoint to it
        const ImVec2 end_pos =
            should_snap
                ? GetScreenSpacePinCoordinates(editor, editor.Pins.Pool[g->HoveredPinIdx.Value()])
                : g->MousePos;

        const LinkBezierData link_data = GetLinkRenderable(
            start_pos, end_pos, start_pin.Type, g->Style.LinkLineSegmentsPerLength);
#if IMGUI_VERSION_NUM < 18000
        g->CanvasDrawList->AddBezierCurve(
#else
        g->CanvasDrawList->AddBezierCubic(
#endif
            link_data.Bezier.P0,
            link_data.Bezier.P1,
            link_data.Bezier.P2,
            link_data.Bezier.P3,
            g->Style.Colors[ColorStyle_Link],
            g->Style.LinkThickness,
            link_data.NumSegments);

        const bool link_creation_on_snap =
            g->HoveredPinIdx.HasValue() && (editor.Pins.Pool[g->HoveredPinIdx.Value()].Flags &
                                            AttributeFlags_EnableLinkCreationOnSnap);

        if (!should_snap)
        {
            editor.ClickInteraction.LinkCreation.EndPinIdx.Reset();
        }

        const bool create_link = should_snap && (g->LeftMouseReleased || link_creation_on_snap);

        if (create_link && !maybe_duplicate_link_idx.HasValue())
        {
            // Avoid send OnLinkCreated() events every frame if the snap link is not saved
            // (only applies for EnableLinkCreationOnSnap)
            if (!g->LeftMouseReleased &&
                editor.ClickInteraction.LinkCreation.EndPinIdx == g->HoveredPinIdx)
            {
                break;
            }

            g->ElementStateChange |= ElementStateChange_LinkCreated;
            editor.ClickInteraction.LinkCreation.EndPinIdx = g->HoveredPinIdx.Value();
        }

        if (g->LeftMouseReleased)
        {
            editor.ClickInteraction.Type = ClickInteractionType_None;
            if (!create_link)
            {
                g->ElementStateChange |= ElementStateChange_LinkDropped;
            }
        }
    }
    break;
    case ClickInteractionType_Panning:
    {
        const bool dragging = g->AltMouseDragging;

        if (dragging)
        {
            editor.Panning += ImGui::GetIO().MouseDelta;
        }
        else
        {
            editor.ClickInteraction.Type = ClickInteractionType_None;
        }
    }
    break;
    case ClickInteractionType_None:
        break;
    default:
        assert(!"Unreachable code!");
        break;
    }
}

void ResolveOccludedPins(const EditorContext& editor, ImVector<int>& occluded_pin_indices)
{
    const ImVector<int>& depth_stack = editor.NodeDepthOrder;

    occluded_pin_indices.resize(0);

    if (depth_stack.Size < 2)
    {
        return;
    }

    // For each node in the depth stack
    for (int depth_idx = 0; depth_idx < (depth_stack.Size - 1); ++depth_idx)
    {
        const NodeData& node_below = editor.Nodes.Pool[depth_stack[depth_idx]];

        // Iterate over the rest of the depth stack to find nodes overlapping the pins
        for (int next_depth_idx = depth_idx + 1; next_depth_idx < depth_stack.Size;
             ++next_depth_idx)
        {
            const ImRect& rect_above = editor.Nodes.Pool[depth_stack[next_depth_idx]].Rect;

            // Iterate over each pin
            for (int idx = 0; idx < node_below.PinIndices.Size; ++idx)
            {
                const int pin_idx = node_below.PinIndices[idx];
                const ImVec2& pin_pos = editor.Pins.Pool[pin_idx].Pos;

                if (rect_above.Contains(pin_pos))
                {
                    occluded_pin_indices.push_back(pin_idx);
                }
            }
        }
    }
}

OptionalIndex ResolveHoveredPin(
    const ObjectPool<PinData>& pins,
    const ImVector<int>& occluded_pin_indices)
{
    float smallest_distance = FLT_MAX;
    OptionalIndex pin_idx_with_smallest_distance;

    const float hover_radius_sqr = g->Style.PinHoverRadius * g->Style.PinHoverRadius;

    for (int idx = 0; idx < pins.Pool.Size; ++idx)
    {
        if (!pins.InUse[idx])
        {
            continue;
        }

        if (occluded_pin_indices.contains(idx))
        {
            continue;
        }

        const ImVec2& pin_pos = pins.Pool[idx].Pos;
        const float distance_sqr = ImLengthSqr(pin_pos - g->MousePos);

        // TODO: g->Style.PinHoverRadius needs to be copied into pin data and the pin-local value
        // used here. This is no longer called in BeginAttribute/EndAttribute scope and the detected
        // pin might have a different hover radius than what the user had when calling
        // BeginAttribute/EndAttribute.
        if (distance_sqr < hover_radius_sqr && distance_sqr < smallest_distance)
        {
            smallest_distance = distance_sqr;
            pin_idx_with_smallest_distance = idx;
        }
    }

    return pin_idx_with_smallest_distance;
}

OptionalIndex ResolveHoveredNode(const ImVector<int>& depth_stack)
{
    if (g->NodeIndicesOverlappingWithMouse.size() == 0)
    {
        return OptionalIndex();
    }

    if (g->NodeIndicesOverlappingWithMouse.size() == 1)
    {
        return OptionalIndex(g->NodeIndicesOverlappingWithMouse[0]);
    }

    int largest_depth_idx = -1;
    int node_idx_on_top = -1;

    for (int i = 0; i < g->NodeIndicesOverlappingWithMouse.size(); ++i)
    {
        const int node_idx = g->NodeIndicesOverlappingWithMouse[i];
        for (int depth_idx = 0; depth_idx < depth_stack.size(); ++depth_idx)
        {
            if (depth_stack[depth_idx] == node_idx && (depth_idx > largest_depth_idx))
            {
                largest_depth_idx = depth_idx;
                node_idx_on_top = node_idx;
            }
        }
    }

    assert(node_idx_on_top != -1);
    return OptionalIndex(node_idx_on_top);
}

OptionalIndex ResolveHoveredLink(const ObjectPool<LinkData>& links, const ObjectPool<PinData>& pins)
{
    float smallest_distance = FLT_MAX;
    OptionalIndex link_idx_with_smallest_distance;

    // There are two ways a link can be detected as "hovered".
    // 1. The link is within hover distance to the mouse. The closest such link is selected as being
    // hovered over.
    // 2. If the link is connected to the currently hovered pin.
    //
    // The latter is a requirement for link detaching with drag click to work, as both a link and
    // pin are required to be hovered over for the feature to work.

    for (int idx = 0; idx < links.Pool.Size; ++idx)
    {
        if (!links.InUse[idx])
        {
            continue;
        }

        const LinkData& link = links.Pool[idx];
        const PinData& start_pin = pins.Pool[link.StartPinIdx];
        const PinData& end_pin = pins.Pool[link.EndPinIdx];

        if (g->HoveredPinIdx == link.StartPinIdx || g->HoveredPinIdx == link.EndPinIdx)
        {
            return idx;
        }

        // TODO: the calculated LinkBezierDatas could be cached since we generate them again when
        // rendering the links

        const LinkBezierData link_data = GetLinkRenderable(
            start_pin.Pos, end_pin.Pos, start_pin.Type, g->Style.LinkLineSegmentsPerLength);

        // The distance test
        {
            const ImRect link_rect = GetContainingRectForBezierCurve(link_data.Bezier);

            // First, do a simple bounding box test against the box containing the link
            // to see whether calculating the distance to the link is worth doing.
            if (link_rect.Contains(g->MousePos))
            {
                const float distance =
                    GetDistanceToCubicBezier(g->MousePos, link_data.Bezier, link_data.NumSegments);

                // TODO: g->Style.LinkHoverDistance could be also copied into LinkData, since
                // we're not calling this function in the same scope as imnodes::Link(). The
                // rendered/detected link might have a different hover distance than what the user
                // had specified when calling Link()
                if (distance < g->Style.LinkHoverDistance)
                {
                    smallest_distance = distance;
                    link_idx_with_smallest_distance = idx;
                }
            }
        }
    }

    return link_idx_with_smallest_distance;
}

// [SECTION] render helpers

inline ImVec2 ScreenSpaceToGridSpace(const EditorContext& editor, const ImVec2& v)
{
    return v - g->CanvasOriginScreenSpace - editor.Panning;
}

inline ImVec2 GridSpaceToScreenSpace(const EditorContext& editor, const ImVec2& v)
{
    return v + g->CanvasOriginScreenSpace + editor.Panning;
}

inline ImVec2 GridSpaceToEditorSpace(const EditorContext& editor, const ImVec2& v)
{
    return v + editor.Panning;
}

inline ImVec2 EditorSpaceToGridSpace(const EditorContext& editor, const ImVec2& v)
{
    return v - editor.Panning;
}

inline ImVec2 EditorSpaceToScreenSpace(const ImVec2& v) { return g->CanvasOriginScreenSpace + v; }

inline ImRect GetItemRect() { return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()); }

inline ImVec2 GetNodeTitleBarOrigin(const NodeData& node)
{
    return node.Origin + node.LayoutStyle.Padding;
}

inline ImVec2 GetNodeContentOrigin(const NodeData& node)
{
    const ImVec2 title_bar_height =
        ImVec2(0.f, node.TitleBarContentRect.GetHeight() + 2.0f * node.LayoutStyle.Padding.y);
    return node.Origin + title_bar_height + node.LayoutStyle.Padding;
}

inline ImRect GetNodeTitleRect(const NodeData& node)
{
    ImRect expanded_title_rect = node.TitleBarContentRect;
    expanded_title_rect.Expand(node.LayoutStyle.Padding);

    return ImRect(
        expanded_title_rect.Min,
        expanded_title_rect.Min + ImVec2(node.Rect.GetWidth(), 0.f) +
            ImVec2(0.f, expanded_title_rect.GetHeight()));
}

void DrawGrid(EditorContext& editor, const ImVec2& canvas_size)
{
    const ImVec2 offset = editor.Panning;

    for (float x = fmodf(offset.x, g->Style.GridSpacing); x < canvas_size.x;
         x += g->Style.GridSpacing)
    {
        g->CanvasDrawList->AddLine(
            EditorSpaceToScreenSpace(ImVec2(x, 0.0f)),
            EditorSpaceToScreenSpace(ImVec2(x, canvas_size.y)),
            g->Style.Colors[ColorStyle_GridLine]);
    }

    for (float y = fmodf(offset.y, g->Style.GridSpacing); y < canvas_size.y;
         y += g->Style.GridSpacing)
    {
        g->CanvasDrawList->AddLine(
            EditorSpaceToScreenSpace(ImVec2(0.0f, y)),
            EditorSpaceToScreenSpace(ImVec2(canvas_size.x, y)),
            g->Style.Colors[ColorStyle_GridLine]);
    }
}

struct QuadOffsets
{
    ImVec2 TopLeft, BottomLeft, BottomRight, TopRight;
};

QuadOffsets CalculateQuadOffsets(const float side_length)
{
    const float half_side = 0.5f * side_length;

    QuadOffsets offset;

    offset.TopLeft = ImVec2(-half_side, half_side);
    offset.BottomLeft = ImVec2(-half_side, -half_side);
    offset.BottomRight = ImVec2(half_side, -half_side);
    offset.TopRight = ImVec2(half_side, half_side);

    return offset;
}

struct TriangleOffsets
{
    ImVec2 TopLeft, BottomLeft, Right;
};

TriangleOffsets CalculateTriangleOffsets(const float side_length)
{
    // Calculates the Vec2 offsets from an equilateral triangle's midpoint to
    // its vertices. Here is how the left_offset and right_offset are
    // calculated.
    //
    // For an equilateral triangle of side length s, the
    // triangle's height, h, is h = s * sqrt(3) / 2.
    //
    // The length from the base to the midpoint is (1 / 3) * h. The length from
    // the midpoint to the triangle vertex is (2 / 3) * h.
    const float sqrt_3 = sqrtf(3.0f);
    const float left_offset = -0.1666666666667f * sqrt_3 * side_length;
    const float right_offset = 0.333333333333f * sqrt_3 * side_length;
    const float vertical_offset = 0.5f * side_length;

    TriangleOffsets offset;
    offset.TopLeft = ImVec2(left_offset, vertical_offset);
    offset.BottomLeft = ImVec2(left_offset, -vertical_offset);
    offset.Right = ImVec2(right_offset, 0.f);

    return offset;
}

void DrawPinShape(const ImVec2& pin_pos, const PinData& pin, const ImU32 pin_color)
{
    static const int CIRCLE_NUM_SEGMENTS = 8;

    switch (pin.Shape)
    {
    case PinShape_Circle:
    {
        g->CanvasDrawList->AddCircle(
            pin_pos,
            g->Style.PinCircleRadius,
            pin_color,
            CIRCLE_NUM_SEGMENTS,
            g->Style.PinLineThickness);
    }
    break;
    case PinShape_CircleFilled:
    {
        g->CanvasDrawList->AddCircleFilled(
            pin_pos, g->Style.PinCircleRadius, pin_color, CIRCLE_NUM_SEGMENTS);
    }
    break;
    case PinShape_Quad:
    {
        const QuadOffsets offset = CalculateQuadOffsets(g->Style.PinQuadSideLength);
        g->CanvasDrawList->AddQuad(
            pin_pos + offset.TopLeft,
            pin_pos + offset.BottomLeft,
            pin_pos + offset.BottomRight,
            pin_pos + offset.TopRight,
            pin_color,
            g->Style.PinLineThickness);
    }
    break;
    case PinShape_QuadFilled:
    {
        const QuadOffsets offset = CalculateQuadOffsets(g->Style.PinQuadSideLength);
        g->CanvasDrawList->AddQuadFilled(
            pin_pos + offset.TopLeft,
            pin_pos + offset.BottomLeft,
            pin_pos + offset.BottomRight,
            pin_pos + offset.TopRight,
            pin_color);
    }
    break;
    case PinShape_Triangle:
    {
        const TriangleOffsets offset = CalculateTriangleOffsets(g->Style.PinTriangleSideLength);
        g->CanvasDrawList->AddTriangle(
            pin_pos + offset.TopLeft,
            pin_pos + offset.BottomLeft,
            pin_pos + offset.Right,
            pin_color,
            // NOTE: for some weird reason, the line drawn by AddTriangle is
            // much thinner than the lines drawn by AddCircle or AddQuad.
            // Multiplying the line thickness by two seemed to solve the
            // problem at a few different thickness values.
            2.f * g->Style.PinLineThickness);
    }
    break;
    case PinShape_TriangleFilled:
    {
        const TriangleOffsets offset = CalculateTriangleOffsets(g->Style.PinTriangleSideLength);
        g->CanvasDrawList->AddTriangleFilled(
            pin_pos + offset.TopLeft,
            pin_pos + offset.BottomLeft,
            pin_pos + offset.Right,
            pin_color);
    }
    break;
    default:
        assert(!"Invalid PinShape value!");
        break;
    }
}

void DrawPin(EditorContext& editor, const int pin_idx, const bool left_mouse_clicked)
{
    PinData& pin = editor.Pins.Pool[pin_idx];
    const ImRect& parent_node_rect = editor.Nodes.Pool[pin.ParentNodeIdx].Rect;

    pin.Pos = GetScreenSpacePinCoordinates(parent_node_rect, pin.AttributeRect, pin.Type);

    ImU32 pin_color = pin.ColorStyle.Background;

    const bool pin_hovered = g->HoveredPinIdx == pin_idx &&
                             editor.ClickInteraction.Type != ClickInteractionType_BoxSelection;

    if (pin_hovered)
    {
        g->HoveredPinIdx = pin_idx;
        g->HoveredPinFlags = pin.Flags;
        pin_color = pin.ColorStyle.Hovered;

        if (left_mouse_clicked)
        {
            BeginLinkCreation(editor, pin_idx);
        }
    }

    DrawPinShape(pin.Pos, pin, pin_color);
}

void DrawNode(EditorContext& editor, const int node_idx)
{
    const NodeData& node = editor.Nodes.Pool[node_idx];
    ImGui::SetCursorPos(node.Origin + editor.Panning);

    const bool node_hovered = g->HoveredNodeIdx == node_idx &&
                              editor.ClickInteraction.Type != ClickInteractionType_BoxSelection;

    ImU32 node_background = node.ColorStyle.Background;
    ImU32 titlebar_background = node.ColorStyle.Titlebar;

    if (editor.SelectedNodeIndices.contains(node_idx))
    {
        node_background = node.ColorStyle.BackgroundSelected;
        titlebar_background = node.ColorStyle.TitlebarSelected;
    }
    else if (node_hovered)
    {
        node_background = node.ColorStyle.BackgroundHovered;
        titlebar_background = node.ColorStyle.TitlebarHovered;
    }

    {
        // node base
        g->CanvasDrawList->AddRectFilled(
            node.Rect.Min, node.Rect.Max, node_background, node.LayoutStyle.CornerRounding);

        // title bar:
        if (node.TitleBarContentRect.GetHeight() > 0.f)
        {
            ImRect title_bar_rect = GetNodeTitleRect(node);

#if IMGUI_VERSION_NUM < 18200
            g->CanvasDrawList->AddRectFilled(
                title_bar_rect.Min,
                title_bar_rect.Max,
                titlebar_background,
                node.LayoutStyle.CornerRounding,
                ImDrawCornerFlags_Top);
#else
            g->CanvasDrawList->AddRectFilled(
                title_bar_rect.Min,
                title_bar_rect.Max,
                titlebar_background,
                node.LayoutStyle.CornerRounding,
                ImDrawFlags_RoundCornersTop);

#endif
        }

        if ((g->Style.Flags & StyleFlags_NodeOutline) != 0)
        {
#if IMGUI_VERSION_NUM < 18200
            g->CanvasDrawList->AddRect(
                node.Rect.Min,
                node.Rect.Max,
                node.ColorStyle.Outline,
                node.LayoutStyle.CornerRounding,
                ImDrawCornerFlags_All,
                node.LayoutStyle.BorderThickness);
#else
            g->CanvasDrawList->AddRect(
                node.Rect.Min,
                node.Rect.Max,
                node.ColorStyle.Outline,
                node.LayoutStyle.CornerRounding,
                ImDrawFlags_RoundCornersAll,
                node.LayoutStyle.BorderThickness);
#endif
        }
    }

    for (int i = 0; i < node.PinIndices.size(); ++i)
    {
        DrawPin(editor, node.PinIndices[i], g->LeftMouseClicked);
    }

    if (node_hovered)
    {
        g->HoveredNodeIdx = node_idx;
        const bool node_free_to_move = g->InteractiveNodeIdx != node_idx;
        if (g->LeftMouseClicked && node_free_to_move)
        {
            BeginNodeSelection(editor, node_idx);
        }
    }
}

void DrawLink(EditorContext& editor, const int link_idx)
{
    const LinkData& link = editor.Links.Pool[link_idx];
    const PinData& start_pin = editor.Pins.Pool[link.StartPinIdx];
    const PinData& end_pin = editor.Pins.Pool[link.EndPinIdx];

    const LinkBezierData link_data = GetLinkRenderable(
        start_pin.Pos, end_pin.Pos, start_pin.Type, g->Style.LinkLineSegmentsPerLength);

    const bool link_hovered = g->HoveredLinkIdx == link_idx &&
                              editor.ClickInteraction.Type != ClickInteractionType_BoxSelection;

    if (link_hovered)
    {
        g->HoveredLinkIdx = link_idx;
        if (g->LeftMouseClicked)
        {
            BeginLinkInteraction(editor, link_idx);
        }
    }

    // It's possible for a link to be deleted in begin_link_interaction. A user
    // may detach a link, resulting in the link wire snapping to the mouse
    // position.
    //
    // In other words, skip rendering the link if it was deleted.
    if (g->DeletedLinkIdx == link_idx)
    {
        return;
    }

    ImU32 link_color = link.ColorStyle.Base;
    if (editor.SelectedLinkIndices.contains(link_idx))
    {
        link_color = link.ColorStyle.Selected;
    }
    else if (link_hovered)
    {
        link_color = link.ColorStyle.Hovered;
    }

#if IMGUI_VERSION_NUM < 18000
    g->CanvasDrawList->AddBezierCurve(
#else
    g->CanvasDrawList->AddBezierCubic(
#endif
        link_data.Bezier.P0,
        link_data.Bezier.P1,
        link_data.Bezier.P2,
        link_data.Bezier.P3,
        link_color,
        g->Style.LinkThickness,
        link_data.NumSegments);
}

void BeginPinAttribute(
    const int id,
    const AttributeType type,
    const PinShape shape,
    const int node_idx)
{
    // Make sure to call BeginNode() before calling
    // BeginAttribute()
    assert(g->CurrentScope == Scope_Node);
    g->CurrentScope = Scope_Attribute;

    ImGui::BeginGroup();
    ImGui::PushID(id);

    EditorContext& editor = EditorContextGet();

    g->CurrentAttributeId = id;

    const int pin_idx = ObjectPoolFindOrCreateIndex(editor.Pins, id);
    g->CurrentPinIdx = pin_idx;
    PinData& pin = editor.Pins.Pool[pin_idx];
    pin.Id = id;
    pin.ParentNodeIdx = node_idx;
    pin.Type = type;
    pin.Shape = shape;
    pin.Flags = g->CurrentAttributeFlags;
    pin.ColorStyle.Background = g->Style.Colors[ColorStyle_Pin];
    pin.ColorStyle.Hovered = g->Style.Colors[ColorStyle_PinHovered];
}

void EndPinAttribute()
{
    assert(g->CurrentScope == Scope_Attribute);
    g->CurrentScope = Scope_Node;

    ImGui::PopID();
    ImGui::EndGroup();

    if (ImGui::IsItemActive())
    {
        g->ActiveAttribute = true;
        g->ActiveAttributeId = g->CurrentAttributeId;
        g->InteractiveNodeIdx = g->CurrentNodeIdx;
    }

    EditorContext& editor = EditorContextGet();
    PinData& pin = editor.Pins.Pool[g->CurrentPinIdx];
    NodeData& node = editor.Nodes.Pool[g->CurrentNodeIdx];
    pin.AttributeRect = GetItemRect();
    node.PinIndices.push_back(g->CurrentPinIdx);
}

void Initialize(Context* context)
{
    context->CanvasOriginScreenSpace = ImVec2(0.0f, 0.0f);
    context->CanvasRectScreenSpace = ImRect(ImVec2(0.f, 0.f), ImVec2(0.f, 0.f));
    context->CurrentScope = Scope_None;

    context->CurrentPinIdx = INT_MAX;
    context->CurrentNodeIdx = INT_MAX;

    context->DefaultEditorCtx = EditorContextCreate();
    EditorContextSet(g->DefaultEditorCtx);

    context->CurrentAttributeFlags = AttributeFlags_None;
    context->AttributeFlagStack.push_back(g->CurrentAttributeFlags);

    StyleColorsDark();
}

void Shutdown(Context* ctx) { EditorContextFree(ctx->DefaultEditorCtx); }
} // namespace

// [SECTION] API implementation

IO::EmulateThreeButtonMouse::EmulateThreeButtonMouse() : Modifier(NULL) {}

IO::LinkDetachWithModifierClick::LinkDetachWithModifierClick() : Modifier(NULL) {}

IO::IO()
    : EmulateThreeButtonMouse(), LinkDetachWithModifierClick(),
      AltMouseButton(ImGuiMouseButton_Middle)
{
}

Style::Style()
    : GridSpacing(32.f), NodeCornerRounding(4.f), NodePaddingHorizontal(8.f),
      NodePaddingVertical(8.f), NodeBorderThickness(1.f), LinkThickness(3.f),
      LinkLineSegmentsPerLength(0.1f), LinkHoverDistance(10.f), PinCircleRadius(4.f),
      PinQuadSideLength(7.f), PinTriangleSideLength(9.5), PinLineThickness(1.f),
      PinHoverRadius(10.f), PinOffset(0.f),
      Flags(StyleFlags(StyleFlags_NodeOutline | StyleFlags_GridLines)), Colors()
{
}

Context* CreateContext()
{
    Context* ctx = IM_NEW(Context)();
    if (g == NULL)
        SetCurrentContext(ctx);
    Initialize(ctx);
    return ctx;
}

void DestroyContext(Context* ctx)
{
    if (ctx == NULL)
        ctx = g;
    Shutdown(ctx);
    if (g == ctx)
        SetCurrentContext(NULL);
    IM_DELETE(ctx);
}

Context* GetCurrentContext() { return g; }

void SetCurrentContext(Context* ctx) { g = ctx; }

EditorContext* EditorContextCreate()
{
    void* mem = ImGui::MemAlloc(sizeof(EditorContext));
    new (mem) EditorContext();
    return (EditorContext*)mem;
}

void EditorContextFree(EditorContext* ctx)
{
    ctx->~EditorContext();
    ImGui::MemFree(ctx);
}

void EditorContextSet(EditorContext* ctx) { g->EditorCtx = ctx; }

ImVec2 EditorContextGetPanning()
{
    const EditorContext& editor = EditorContextGet();
    return editor.Panning;
}

void EditorContextResetPanning(const ImVec2& pos)
{
    EditorContext& editor = EditorContextGet();
    editor.Panning = pos;
}

void EditorContextMoveToNode(const int node_id)
{
    EditorContext& editor = EditorContextGet();
    NodeData& node = ObjectPoolFindOrCreateObject(editor.Nodes, node_id);

    editor.Panning.x = -node.Origin.x;
    editor.Panning.y = -node.Origin.y;
}

void SetImGuiContext(ImGuiContext* ctx) { ImGui::SetCurrentContext(ctx); }

IO& GetIO() { return g->Io; }

Style& GetStyle() { return g->Style; }

void StyleColorsDark()
{
    g->Style.Colors[ColorStyle_NodeBackground] = IM_COL32(50, 50, 50, 255);
    g->Style.Colors[ColorStyle_NodeBackgroundHovered] = IM_COL32(75, 75, 75, 255);
    g->Style.Colors[ColorStyle_NodeBackgroundSelected] = IM_COL32(75, 75, 75, 255);
    g->Style.Colors[ColorStyle_NodeOutline] = IM_COL32(100, 100, 100, 255);
    // title bar colors match ImGui's titlebg colors
    g->Style.Colors[ColorStyle_TitleBar] = IM_COL32(41, 74, 122, 255);
    g->Style.Colors[ColorStyle_TitleBarHovered] = IM_COL32(66, 150, 250, 255);
    g->Style.Colors[ColorStyle_TitleBarSelected] = IM_COL32(66, 150, 250, 255);
    // link colors match ImGui's slider grab colors
    g->Style.Colors[ColorStyle_Link] = IM_COL32(61, 133, 224, 200);
    g->Style.Colors[ColorStyle_LinkHovered] = IM_COL32(66, 150, 250, 255);
    g->Style.Colors[ColorStyle_LinkSelected] = IM_COL32(66, 150, 250, 255);
    // pin colors match ImGui's button colors
    g->Style.Colors[ColorStyle_Pin] = IM_COL32(53, 150, 250, 180);
    g->Style.Colors[ColorStyle_PinHovered] = IM_COL32(53, 150, 250, 255);

    g->Style.Colors[ColorStyle_BoxSelector] = IM_COL32(61, 133, 224, 30);
    g->Style.Colors[ColorStyle_BoxSelectorOutline] = IM_COL32(61, 133, 224, 150);

    g->Style.Colors[ColorStyle_GridBackground] = IM_COL32(40, 40, 50, 200);
    g->Style.Colors[ColorStyle_GridLine] = IM_COL32(200, 200, 200, 40);
}

void StyleColorsClassic()
{
    g->Style.Colors[ColorStyle_NodeBackground] = IM_COL32(50, 50, 50, 255);
    g->Style.Colors[ColorStyle_NodeBackgroundHovered] = IM_COL32(75, 75, 75, 255);
    g->Style.Colors[ColorStyle_NodeBackgroundSelected] = IM_COL32(75, 75, 75, 255);
    g->Style.Colors[ColorStyle_NodeOutline] = IM_COL32(100, 100, 100, 255);
    g->Style.Colors[ColorStyle_TitleBar] = IM_COL32(69, 69, 138, 255);
    g->Style.Colors[ColorStyle_TitleBarHovered] = IM_COL32(82, 82, 161, 255);
    g->Style.Colors[ColorStyle_TitleBarSelected] = IM_COL32(82, 82, 161, 255);
    g->Style.Colors[ColorStyle_Link] = IM_COL32(255, 255, 255, 100);
    g->Style.Colors[ColorStyle_LinkHovered] = IM_COL32(105, 99, 204, 153);
    g->Style.Colors[ColorStyle_LinkSelected] = IM_COL32(105, 99, 204, 153);
    g->Style.Colors[ColorStyle_Pin] = IM_COL32(89, 102, 156, 170);
    g->Style.Colors[ColorStyle_PinHovered] = IM_COL32(102, 122, 179, 200);
    g->Style.Colors[ColorStyle_BoxSelector] = IM_COL32(82, 82, 161, 100);
    g->Style.Colors[ColorStyle_BoxSelectorOutline] = IM_COL32(82, 82, 161, 255);
    g->Style.Colors[ColorStyle_GridBackground] = IM_COL32(40, 40, 50, 200);
    g->Style.Colors[ColorStyle_GridLine] = IM_COL32(200, 200, 200, 40);
}

void StyleColorsLight()
{
    g->Style.Colors[ColorStyle_NodeBackground] = IM_COL32(240, 240, 240, 255);
    g->Style.Colors[ColorStyle_NodeBackgroundHovered] = IM_COL32(240, 240, 240, 255);
    g->Style.Colors[ColorStyle_NodeBackgroundSelected] = IM_COL32(240, 240, 240, 255);
    g->Style.Colors[ColorStyle_NodeOutline] = IM_COL32(100, 100, 100, 255);
    g->Style.Colors[ColorStyle_TitleBar] = IM_COL32(248, 248, 248, 255);
    g->Style.Colors[ColorStyle_TitleBarHovered] = IM_COL32(209, 209, 209, 255);
    g->Style.Colors[ColorStyle_TitleBarSelected] = IM_COL32(209, 209, 209, 255);
    // original imgui values: 66, 150, 250
    g->Style.Colors[ColorStyle_Link] = IM_COL32(66, 150, 250, 100);
    // original imgui values: 117, 138, 204
    g->Style.Colors[ColorStyle_LinkHovered] = IM_COL32(66, 150, 250, 242);
    g->Style.Colors[ColorStyle_LinkSelected] = IM_COL32(66, 150, 250, 242);
    // original imgui values: 66, 150, 250
    g->Style.Colors[ColorStyle_Pin] = IM_COL32(66, 150, 250, 160);
    g->Style.Colors[ColorStyle_PinHovered] = IM_COL32(66, 150, 250, 255);
    g->Style.Colors[ColorStyle_BoxSelector] = IM_COL32(90, 170, 250, 30);
    g->Style.Colors[ColorStyle_BoxSelectorOutline] = IM_COL32(90, 170, 250, 150);
    g->Style.Colors[ColorStyle_GridBackground] = IM_COL32(225, 225, 225, 255);
    g->Style.Colors[ColorStyle_GridLine] = IM_COL32(180, 180, 180, 100);
    g->Style.Flags = StyleFlags(StyleFlags_None);
}

void BeginNodeEditor()
{
    assert(g->CurrentScope == Scope_None);
    g->CurrentScope = Scope_Editor;

    // Reset state from previous pass

    EditorContext& editor = EditorContextGet();
    ObjectPoolReset(editor.Nodes);
    ObjectPoolReset(editor.Pins);
    ObjectPoolReset(editor.Links);

    g->HoveredNodeIdx.Reset();
    g->InteractiveNodeIdx.Reset();
    g->HoveredLinkIdx.Reset();
    g->HoveredPinIdx.Reset();
    g->HoveredPinFlags = AttributeFlags_None;
    g->DeletedLinkIdx.Reset();
    g->SnapLinkIdx.Reset();

    g->NodeIndicesOverlappingWithMouse.clear();

    g->ElementStateChange = ElementStateChange_None;

    g->MousePos = ImGui::GetIO().MousePos;
    g->LeftMouseClicked = ImGui::IsMouseClicked(0);
    g->LeftMouseReleased = ImGui::IsMouseReleased(0);
    g->AltMouseClicked = (g->Io.EmulateThreeButtonMouse.Modifier != NULL &&
                          *g->Io.EmulateThreeButtonMouse.Modifier && g->LeftMouseClicked) ||
                         ImGui::IsMouseClicked(g->Io.AltMouseButton);
    g->LeftMouseDragging = ImGui::IsMouseDragging(0, 0.0f);
    g->AltMouseDragging = (g->Io.EmulateThreeButtonMouse.Modifier != NULL && g->LeftMouseDragging &&
                           (*g->Io.EmulateThreeButtonMouse.Modifier)) ||
                          ImGui::IsMouseDragging(g->Io.AltMouseButton, 0.0f);

    g->ActiveAttribute = false;

    ImGui::BeginGroup();
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, g->Style.Colors[ColorStyle_GridBackground]);
        ImGui::BeginChild(
            "scrolling_region",
            ImVec2(0.f, 0.f),
            true,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollWithMouse);
        g->CanvasOriginScreenSpace = ImGui::GetCursorScreenPos();

        // NOTE: we have to fetch the canvas draw list *after* we call
        // BeginChild(), otherwise the ImGui UI elements are going to be
        // rendered into the parent window draw list.
        DrawListSet(ImGui::GetWindowDrawList());

        {
            const ImVec2 canvas_size = ImGui::GetWindowSize();
            g->CanvasRectScreenSpace = ImRect(
                EditorSpaceToScreenSpace(ImVec2(0.f, 0.f)), EditorSpaceToScreenSpace(canvas_size));

            if (g->Style.Flags & StyleFlags_GridLines)
            {
                DrawGrid(editor, canvas_size);
            }
        }
    }
}

void EndNodeEditor()
{
    assert(g->CurrentScope == Scope_Editor);
    g->CurrentScope = Scope_None;

    EditorContext& editor = EditorContextGet();

    // Detect which UI element is being hovered over. Detection is done in a hierarchical fashion,
    // because a UI element being hovered excludes any other as being hovered over.

    if (MouseInCanvas())
    {
        // Pins needs some special care. We need to check the depth stack to see which pins are
        // being occluded by other nodes.
        ResolveOccludedPins(editor, g->OccludedPinIndices);

        g->HoveredPinIdx = ResolveHoveredPin(editor.Pins, g->OccludedPinIndices);

        if (!g->HoveredPinIdx.HasValue())
        {
            // Resolve which node is actually on top and being hovered using the depth stack.
            g->HoveredNodeIdx = ResolveHoveredNode(editor.NodeDepthOrder);
        }

        // We don't need to check the depth stack for links. If a node occludes a link and is being
        // hovered, then we would not be able to detect the link anyway.
        if (!g->HoveredNodeIdx.HasValue())
        {
            g->HoveredLinkIdx = ResolveHoveredLink(editor.Links, editor.Pins);
        }
    }

    for (int node_idx = 0; node_idx < editor.Nodes.Pool.size(); ++node_idx)
    {
        if (editor.Nodes.InUse[node_idx])
        {
            DrawListActivateNodeBackground(node_idx);
            DrawNode(editor, node_idx);
        }
    }

    // In order to render the links underneath the nodes, we want to first select the bottom draw
    // channel.
    g->CanvasDrawList->ChannelsSetCurrent(0);

    for (int link_idx = 0; link_idx < editor.Links.Pool.size(); ++link_idx)
    {
        if (editor.Links.InUse[link_idx])
        {
            DrawLink(editor, link_idx);
        }
    }

    // Render the click interaction UI elements (partial links, box selector) on top of everything
    // else.

    DrawListAppendClickInteractionChannel();
    DrawListActivateClickInteractionChannel();

    if (g->LeftMouseClicked || g->AltMouseClicked)
    {
        BeginCanvasInteraction(editor);
    }

    ClickInteractionUpdate(editor);

    // At this point, draw commands have been issued for all nodes (and pins). Update the node pool
    // to detect unused node slots and remove those indices from the depth stack before sorting the
    // node draw commands by depth.
    ObjectPoolUpdate(editor.Nodes);
    ObjectPoolUpdate(editor.Pins);

    DrawListSortChannelsByDepth(editor.NodeDepthOrder);

    // After the links have been rendered, the link pool can be updated as well.
    ObjectPoolUpdate(editor.Links);

    // Finally, merge the draw channels
    g->CanvasDrawList->ChannelsMerge();

    // pop style
    ImGui::EndChild();      // end scrolling region
    ImGui::PopStyleColor(); // pop child window background color
    ImGui::PopStyleVar();   // pop window padding
    ImGui::PopStyleVar();   // pop frame padding
    ImGui::EndGroup();
}

void BeginNode(const int node_id)
{
    // Remember to call BeginNodeEditor before calling BeginNode
    assert(g->CurrentScope == Scope_Editor);
    g->CurrentScope = Scope_Node;

    EditorContext& editor = EditorContextGet();

    const int node_idx = ObjectPoolFindOrCreateIndex(editor.Nodes, node_id);
    g->CurrentNodeIdx = node_idx;

    NodeData& node = editor.Nodes.Pool[node_idx];
    node.ColorStyle.Background = g->Style.Colors[ColorStyle_NodeBackground];
    node.ColorStyle.BackgroundHovered = g->Style.Colors[ColorStyle_NodeBackgroundHovered];
    node.ColorStyle.BackgroundSelected = g->Style.Colors[ColorStyle_NodeBackgroundSelected];
    node.ColorStyle.Outline = g->Style.Colors[ColorStyle_NodeOutline];
    node.ColorStyle.Titlebar = g->Style.Colors[ColorStyle_TitleBar];
    node.ColorStyle.TitlebarHovered = g->Style.Colors[ColorStyle_TitleBarHovered];
    node.ColorStyle.TitlebarSelected = g->Style.Colors[ColorStyle_TitleBarSelected];
    node.LayoutStyle.CornerRounding = g->Style.NodeCornerRounding;
    node.LayoutStyle.Padding = ImVec2(g->Style.NodePaddingHorizontal, g->Style.NodePaddingVertical);
    node.LayoutStyle.BorderThickness = g->Style.NodeBorderThickness;

    // ImGui::SetCursorPos sets the cursor position, local to the current widget
    // (in this case, the child object started in BeginNodeEditor). Use
    // ImGui::SetCursorScreenPos to set the screen space coordinates directly.
    ImGui::SetCursorPos(GridSpaceToEditorSpace(editor, GetNodeTitleBarOrigin(node)));

    DrawListAddNode(node_idx);
    DrawListActivateCurrentNodeForeground();

    ImGui::PushID(node.Id);
    ImGui::BeginGroup();
}

void EndNode()
{
    assert(g->CurrentScope == Scope_Node);
    g->CurrentScope = Scope_Editor;

    EditorContext& editor = EditorContextGet();

    // The node's rectangle depends on the ImGui UI group size.
    ImGui::EndGroup();
    ImGui::PopID();

    NodeData& node = editor.Nodes.Pool[g->CurrentNodeIdx];
    node.Rect = GetItemRect();
    node.Rect.Expand(node.LayoutStyle.Padding);

    if (node.Rect.Contains(g->MousePos))
    {
        g->NodeIndicesOverlappingWithMouse.push_back(g->CurrentNodeIdx);
    }
}

ImVec2 GetNodeDimensions(int node_id)
{
    EditorContext& editor = EditorContextGet();
    const int node_idx = ObjectPoolFind(editor.Nodes, node_id);
    assert(node_idx != -1); // invalid node_id
    const NodeData& node = editor.Nodes.Pool[node_idx];
    return node.Rect.GetSize();
}

void BeginNodeTitleBar()
{
    assert(g->CurrentScope == Scope_Node);
    ImGui::BeginGroup();
}

void EndNodeTitleBar()
{
    assert(g->CurrentScope == Scope_Node);
    ImGui::EndGroup();

    EditorContext& editor = EditorContextGet();
    NodeData& node = editor.Nodes.Pool[g->CurrentNodeIdx];
    node.TitleBarContentRect = GetItemRect();

    ImGui::ItemAdd(GetNodeTitleRect(node), ImGui::GetID("title_bar"));

    ImGui::SetCursorPos(GridSpaceToEditorSpace(editor, GetNodeContentOrigin(node)));
}

void BeginInputAttribute(const int id, const PinShape shape)
{
    BeginPinAttribute(id, AttributeType_Input, shape, g->CurrentNodeIdx);
}

void EndInputAttribute() { EndPinAttribute(); }

void BeginOutputAttribute(const int id, const PinShape shape)
{
    BeginPinAttribute(id, AttributeType_Output, shape, g->CurrentNodeIdx);
}

void EndOutputAttribute() { EndPinAttribute(); }

void BeginStaticAttribute(const int id)
{
    // Make sure to call BeginNode() before calling BeginAttribute()
    assert(g->CurrentScope == Scope_Node);
    g->CurrentScope = Scope_Attribute;

    g->CurrentAttributeId = id;

    ImGui::BeginGroup();
    ImGui::PushID(id);
}

void EndStaticAttribute()
{
    // Make sure to call BeginNode() before calling BeginAttribute()
    assert(g->CurrentScope == Scope_Attribute);
    g->CurrentScope = Scope_Node;

    ImGui::PopID();
    ImGui::EndGroup();

    if (ImGui::IsItemActive())
    {
        g->ActiveAttribute = true;
        g->ActiveAttributeId = g->CurrentAttributeId;
        g->InteractiveNodeIdx = g->CurrentNodeIdx;
    }
}

void PushAttributeFlag(AttributeFlags flag)
{
    g->CurrentAttributeFlags |= static_cast<int>(flag);
    g->AttributeFlagStack.push_back(g->CurrentAttributeFlags);
}

void PopAttributeFlag()
{
    // PopAttributeFlag called without a matching PushAttributeFlag!
    // The bottom value is always the default value, pushed in Initialize().
    assert(g->AttributeFlagStack.size() > 1);

    g->AttributeFlagStack.pop_back();
    g->CurrentAttributeFlags = g->AttributeFlagStack.back();
}

void Link(int id, const int start_attr_id, const int end_attr_id)
{
    assert(g->CurrentScope == Scope_Editor);

    EditorContext& editor = EditorContextGet();
    LinkData& link = ObjectPoolFindOrCreateObject(editor.Links, id);
    link.Id = id;
    link.StartPinIdx = ObjectPoolFindOrCreateIndex(editor.Pins, start_attr_id);
    link.EndPinIdx = ObjectPoolFindOrCreateIndex(editor.Pins, end_attr_id);
    link.ColorStyle.Base = g->Style.Colors[ColorStyle_Link];
    link.ColorStyle.Hovered = g->Style.Colors[ColorStyle_LinkHovered];
    link.ColorStyle.Selected = g->Style.Colors[ColorStyle_LinkSelected];

    // Check if this link was created by the current link event
    if ((editor.ClickInteraction.Type == ClickInteractionType_LinkCreation &&
         editor.Pins.Pool[link.EndPinIdx].Flags & AttributeFlags_EnableLinkCreationOnSnap &&
         editor.ClickInteraction.LinkCreation.StartPinIdx == link.StartPinIdx &&
         editor.ClickInteraction.LinkCreation.EndPinIdx == link.EndPinIdx) ||
        (editor.ClickInteraction.LinkCreation.StartPinIdx == link.EndPinIdx &&
         editor.ClickInteraction.LinkCreation.EndPinIdx == link.StartPinIdx))
    {
        g->SnapLinkIdx = ObjectPoolFindOrCreateIndex(editor.Links, id);
    }
}

void PushColorStyle(ColorStyle item, unsigned int color)
{
    g->ColorModifierStack.push_back(ColorStyleElement(g->Style.Colors[item], item));
    g->Style.Colors[item] = color;
}

void PopColorStyle()
{
    assert(g->ColorModifierStack.size() > 0);
    const ColorStyleElement elem = g->ColorModifierStack.back();
    g->Style.Colors[elem.Item] = elem.Color;
    g->ColorModifierStack.pop_back();
}

float& LookupStyleVar(const StyleVar item)
{
    // TODO: once the switch gets too big and unwieldy to work with, we could do
    // a byte-offset lookup into the Style struct, using the StyleVar as an
    // index. This is how ImGui does it.
    float* style_var = 0;
    switch (item)
    {
    case StyleVar_GridSpacing:
        style_var = &g->Style.GridSpacing;
        break;
    case StyleVar_NodeCornerRounding:
        style_var = &g->Style.NodeCornerRounding;
        break;
    case StyleVar_NodePaddingHorizontal:
        style_var = &g->Style.NodePaddingHorizontal;
        break;
    case StyleVar_NodePaddingVertical:
        style_var = &g->Style.NodePaddingVertical;
        break;
    case StyleVar_NodeBorderThickness:
        style_var = &g->Style.NodeBorderThickness;
        break;
    case StyleVar_LinkThickness:
        style_var = &g->Style.LinkThickness;
        break;
    case StyleVar_LinkLineSegmentsPerLength:
        style_var = &g->Style.LinkLineSegmentsPerLength;
        break;
    case StyleVar_LinkHoverDistance:
        style_var = &g->Style.LinkHoverDistance;
        break;
    case StyleVar_PinCircleRadius:
        style_var = &g->Style.PinCircleRadius;
        break;
    case StyleVar_PinQuadSideLength:
        style_var = &g->Style.PinQuadSideLength;
        break;
    case StyleVar_PinTriangleSideLength:
        style_var = &g->Style.PinTriangleSideLength;
        break;
    case StyleVar_PinLineThickness:
        style_var = &g->Style.PinLineThickness;
        break;
    case StyleVar_PinHoverRadius:
        style_var = &g->Style.PinHoverRadius;
        break;
    case StyleVar_PinOffset:
        style_var = &g->Style.PinOffset;
        break;
    default:
        assert(!"Invalid StyleVar value!");
    }

    return *style_var;
}

void PushStyleVar(const StyleVar item, const float value)
{
    float& style_var = LookupStyleVar(item);
    g->StyleModifierStack.push_back(StyleElement(style_var, item));
    style_var = value;
}

void PopStyleVar()
{
    assert(g->StyleModifierStack.size() > 0);
    const StyleElement style_elem = g->StyleModifierStack.back();
    g->StyleModifierStack.pop_back();
    float& style_var = LookupStyleVar(style_elem.Item);
    style_var = style_elem.Value;
}

void SetNodeScreenSpacePos(int node_id, const ImVec2& screen_space_pos)
{
    EditorContext& editor = EditorContextGet();
    NodeData& node = ObjectPoolFindOrCreateObject(editor.Nodes, node_id);
    node.Origin = ScreenSpaceToGridSpace(editor, screen_space_pos);
}

void SetNodeEditorSpacePos(int node_id, const ImVec2& editor_space_pos)
{
    EditorContext& editor = EditorContextGet();
    NodeData& node = ObjectPoolFindOrCreateObject(editor.Nodes, node_id);
    node.Origin = EditorSpaceToGridSpace(editor, editor_space_pos);
}

void SetNodeGridSpacePos(int node_id, const ImVec2& grid_pos)
{
    EditorContext& editor = EditorContextGet();
    NodeData& node = ObjectPoolFindOrCreateObject(editor.Nodes, node_id);
    node.Origin = grid_pos;
}

void SetNodeDraggable(int node_id, const bool draggable)
{
    EditorContext& editor = EditorContextGet();
    NodeData& node = ObjectPoolFindOrCreateObject(editor.Nodes, node_id);
    node.Draggable = draggable;
}

ImVec2 GetNodeScreenSpacePos(const int node_id)
{
    EditorContext& editor = EditorContextGet();
    const int node_idx = ObjectPoolFind(editor.Nodes, node_id);
    assert(node_idx != -1);
    NodeData& node = editor.Nodes.Pool[node_idx];
    return GridSpaceToScreenSpace(editor, node.Origin);
}

ImVec2 GetNodeEditorSpacePos(const int node_id)
{
    EditorContext& editor = EditorContextGet();
    const int node_idx = ObjectPoolFind(editor.Nodes, node_id);
    assert(node_idx != -1);
    NodeData& node = editor.Nodes.Pool[node_idx];
    return GridSpaceToEditorSpace(editor, node.Origin);
}

ImVec2 GetNodeGridSpacePos(int node_id)
{
    EditorContext& editor = EditorContextGet();
    const int node_idx = ObjectPoolFind(editor.Nodes, node_id);
    assert(node_idx != -1);
    NodeData& node = editor.Nodes.Pool[node_idx];
    return node.Origin;
}

bool IsEditorHovered() { return MouseInCanvas(); }

bool IsNodeHovered(int* const node_id)
{
    assert(g->CurrentScope == Scope_None);
    assert(node_id != NULL);

    const bool is_hovered = g->HoveredNodeIdx.HasValue();
    if (is_hovered)
    {
        const EditorContext& editor = EditorContextGet();
        *node_id = editor.Nodes.Pool[g->HoveredNodeIdx.Value()].Id;
    }
    return is_hovered;
}

bool IsLinkHovered(int* const link_id)
{
    assert(g->CurrentScope == Scope_None);
    assert(link_id != NULL);

    const bool is_hovered = g->HoveredLinkIdx.HasValue();
    if (is_hovered)
    {
        const EditorContext& editor = EditorContextGet();
        *link_id = editor.Links.Pool[g->HoveredLinkIdx.Value()].Id;
    }
    return is_hovered;
}

bool IsPinHovered(int* const attr)
{
    assert(g->CurrentScope == Scope_None);
    assert(attr != NULL);

    const bool is_hovered = g->HoveredPinIdx.HasValue();
    if (is_hovered)
    {
        const EditorContext& editor = EditorContextGet();
        *attr = editor.Pins.Pool[g->HoveredPinIdx.Value()].Id;
    }
    return is_hovered;
}

int NumSelectedNodes()
{
    assert(g->CurrentScope == Scope_None);
    const EditorContext& editor = EditorContextGet();
    return editor.SelectedNodeIndices.size();
}

int NumSelectedLinks()
{
    assert(g->CurrentScope == Scope_None);
    const EditorContext& editor = EditorContextGet();
    return editor.SelectedLinkIndices.size();
}

void GetSelectedNodes(int* node_ids)
{
    assert(node_ids != NULL);

    const EditorContext& editor = EditorContextGet();
    for (int i = 0; i < editor.SelectedNodeIndices.size(); ++i)
    {
        const int node_idx = editor.SelectedNodeIndices[i];
        node_ids[i] = editor.Nodes.Pool[node_idx].Id;
    }
}

void GetSelectedLinks(int* link_ids)
{
    assert(link_ids != NULL);

    const EditorContext& editor = EditorContextGet();
    for (int i = 0; i < editor.SelectedLinkIndices.size(); ++i)
    {
        const int link_idx = editor.SelectedLinkIndices[i];
        link_ids[i] = editor.Links.Pool[link_idx].Id;
    }
}

void ClearNodeSelection()
{
    EditorContext& editor = EditorContextGet();
    editor.SelectedNodeIndices.clear();
}

void ClearLinkSelection()
{
    EditorContext& editor = EditorContextGet();
    editor.SelectedLinkIndices.clear();
}

bool IsAttributeActive()
{
    assert((g->CurrentScope & Scope_Node) != 0);

    if (!g->ActiveAttribute)
    {
        return false;
    }

    return g->ActiveAttributeId == g->CurrentAttributeId;
}

bool IsAnyAttributeActive(int* const attribute_id)
{
    assert((g->CurrentScope & (Scope_Node | Scope_Attribute)) == 0);

    if (!g->ActiveAttribute)
    {
        return false;
    }

    if (attribute_id != NULL)
    {
        *attribute_id = g->ActiveAttributeId;
    }

    return true;
}

bool IsLinkStarted(int* const started_at_id)
{
    // Call this function after EndNodeEditor()!
    assert(g->CurrentScope == Scope_None);
    assert(started_at_id != NULL);

    const bool is_started = (g->ElementStateChange & ElementStateChange_LinkStarted) != 0;
    if (is_started)
    {
        const EditorContext& editor = EditorContextGet();
        const int pin_idx = editor.ClickInteraction.LinkCreation.StartPinIdx;
        *started_at_id = editor.Pins.Pool[pin_idx].Id;
    }

    return is_started;
}

bool IsLinkDropped(int* const started_at_id, const bool including_detached_links)
{
    // Call this function after EndNodeEditor()!
    assert(g->CurrentScope == Scope_None);

    const EditorContext& editor = EditorContextGet();

    const bool link_dropped =
        (g->ElementStateChange & ElementStateChange_LinkDropped) != 0 &&
        (including_detached_links ||
         editor.ClickInteraction.LinkCreation.Type != LinkCreationType_FromDetach);

    if (link_dropped && started_at_id)
    {
        const int pin_idx = editor.ClickInteraction.LinkCreation.StartPinIdx;
        *started_at_id = editor.Pins.Pool[pin_idx].Id;
    }

    return link_dropped;
}

bool IsLinkCreated(
    int* const started_at_pin_id,
    int* const ended_at_pin_id,
    bool* const created_from_snap)
{
    assert(g->CurrentScope == Scope_None);
    assert(started_at_pin_id != NULL);
    assert(ended_at_pin_id != NULL);

    const bool is_created = (g->ElementStateChange & ElementStateChange_LinkCreated) != 0;

    if (is_created)
    {
        const EditorContext& editor = EditorContextGet();
        const int start_idx = editor.ClickInteraction.LinkCreation.StartPinIdx;
        const int end_idx = editor.ClickInteraction.LinkCreation.EndPinIdx.Value();
        const PinData& start_pin = editor.Pins.Pool[start_idx];
        const PinData& end_pin = editor.Pins.Pool[end_idx];

        if (start_pin.Type == AttributeType_Output)
        {
            *started_at_pin_id = start_pin.Id;
            *ended_at_pin_id = end_pin.Id;
        }
        else
        {
            *started_at_pin_id = end_pin.Id;
            *ended_at_pin_id = start_pin.Id;
        }

        if (created_from_snap)
        {
            *created_from_snap = editor.ClickInteraction.Type == ClickInteractionType_LinkCreation;
        }
    }

    return is_created;
}

bool IsLinkCreated(
    int* started_at_node_id,
    int* started_at_pin_id,
    int* ended_at_node_id,
    int* ended_at_pin_id,
    bool* created_from_snap)
{
    assert(g->CurrentScope == Scope_None);
    assert(started_at_node_id != NULL);
    assert(started_at_pin_id != NULL);
    assert(ended_at_node_id != NULL);
    assert(ended_at_pin_id != NULL);

    const bool is_created = (g->ElementStateChange & ElementStateChange_LinkCreated) != 0;

    if (is_created)
    {
        const EditorContext& editor = EditorContextGet();
        const int start_idx = editor.ClickInteraction.LinkCreation.StartPinIdx;
        const int end_idx = editor.ClickInteraction.LinkCreation.EndPinIdx.Value();
        const PinData& start_pin = editor.Pins.Pool[start_idx];
        const NodeData& start_node = editor.Nodes.Pool[start_pin.ParentNodeIdx];
        const PinData& end_pin = editor.Pins.Pool[end_idx];
        const NodeData& end_node = editor.Nodes.Pool[end_pin.ParentNodeIdx];

        if (start_pin.Type == AttributeType_Output)
        {
            *started_at_pin_id = start_pin.Id;
            *started_at_node_id = start_node.Id;
            *ended_at_pin_id = end_pin.Id;
            *ended_at_node_id = end_node.Id;
        }
        else
        {
            *started_at_pin_id = end_pin.Id;
            *started_at_node_id = end_node.Id;
            *ended_at_pin_id = start_pin.Id;
            *ended_at_node_id = start_node.Id;
        }

        if (created_from_snap)
        {
            *created_from_snap = editor.ClickInteraction.Type == ClickInteractionType_LinkCreation;
        }
    }

    return is_created;
}

bool IsLinkDestroyed(int* const link_id)
{
    assert(g->CurrentScope == Scope_None);

    const bool link_destroyed = g->DeletedLinkIdx.HasValue();
    if (link_destroyed)
    {
        const EditorContext& editor = EditorContextGet();
        const int link_idx = g->DeletedLinkIdx.Value();
        *link_id = editor.Links.Pool[link_idx].Id;
    }

    return link_destroyed;
}

namespace
{
void NodeLineHandler(EditorContext& editor, const char* line)
{
    int id;
    int x, y;
    if (sscanf(line, "[node.%i", &id) == 1)
    {
        const int node_idx = ObjectPoolFindOrCreateIndex(editor.Nodes, id);
        g->CurrentNodeIdx = node_idx;
        NodeData& node = editor.Nodes.Pool[node_idx];
        node.Id = id;
    }
    else if (sscanf(line, "origin=%i,%i", &x, &y) == 2)
    {
        NodeData& node = editor.Nodes.Pool[g->CurrentNodeIdx];
        node.Origin = ImVec2((float)x, (float)y);
    }
}

void EditorLineHandler(EditorContext& editor, const char* line)
{
    sscanf(line, "panning=%f,%f", &editor.Panning.x, &editor.Panning.y);
}
} // namespace

const char* SaveCurrentEditorStateToIniString(size_t* const data_size)
{
    return SaveEditorStateToIniString(&EditorContextGet(), data_size);
}

const char* SaveEditorStateToIniString(
    const EditorContext* const editor_ptr,
    size_t* const data_size)
{
    assert(editor_ptr != NULL);
    const EditorContext& editor = *editor_ptr;

    g->TextBuffer.clear();
    // TODO: check to make sure that the estimate is the upper bound of element
    g->TextBuffer.reserve(64 * editor.Nodes.Pool.size());

    g->TextBuffer.appendf(
        "[editor]\npanning=%i,%i\n", (int)editor.Panning.x, (int)editor.Panning.y);

    for (int i = 0; i < editor.Nodes.Pool.size(); i++)
    {
        if (editor.Nodes.InUse[i])
        {
            const NodeData& node = editor.Nodes.Pool[i];
            g->TextBuffer.appendf("\n[node.%d]\n", node.Id);
            g->TextBuffer.appendf("origin=%i,%i\n", (int)node.Origin.x, (int)node.Origin.y);
        }
    }

    if (data_size != NULL)
    {
        *data_size = g->TextBuffer.size();
    }

    return g->TextBuffer.c_str();
}

void LoadCurrentEditorStateFromIniString(const char* const data, const size_t data_size)
{
    LoadEditorStateFromIniString(&EditorContextGet(), data, data_size);
}

void LoadEditorStateFromIniString(
    EditorContext* const editor_ptr,
    const char* const data,
    const size_t data_size)
{
    if (data_size == 0u)
    {
        return;
    }

    EditorContext& editor = editor_ptr == NULL ? EditorContextGet() : *editor_ptr;

    char* buf = (char*)ImGui::MemAlloc(data_size + 1);
    const char* buf_end = buf + data_size;
    memcpy(buf, data, data_size);
    buf[data_size] = 0;

    void (*line_handler)(EditorContext&, const char*);
    line_handler = NULL;
    char* line_end = NULL;
    for (char* line = buf; line < buf_end; line = line_end + 1)
    {
        while (*line == '\n' || *line == '\r')
        {
            line++;
        }
        line_end = line;
        while (line_end < buf_end && *line_end != '\n' && *line_end != '\r')
        {
            line_end++;
        }
        line_end[0] = 0;

        if (*line == ';' || *line == '\0')
        {
            continue;
        }

        if (line[0] == '[' && line_end[-1] == ']')
        {
            line_end[-1] = 0;
            if (strncmp(line + 1, "node", 4) == 0)
            {
                line_handler = NodeLineHandler;
            }
            else if (strcmp(line + 1, "editor") == 0)
            {
                line_handler = EditorLineHandler;
            }
        }

        if (line_handler != NULL)
        {
            line_handler(editor, line);
        }
    }
    ImGui::MemFree(buf);
}

void SaveCurrentEditorStateToIniFile(const char* const file_name)
{
    SaveEditorStateToIniFile(&EditorContextGet(), file_name);
}

void SaveEditorStateToIniFile(const EditorContext* const editor, const char* const file_name)
{
    size_t data_size = 0u;
    const char* data = SaveEditorStateToIniString(editor, &data_size);
    FILE* file = ImFileOpen(file_name, "wt");
    if (!file)
    {
        return;
    }

    fwrite(data, sizeof(char), data_size, file);
    fclose(file);
}

void LoadCurrentEditorStateFromIniFile(const char* const file_name)
{
    LoadEditorStateFromIniFile(&EditorContextGet(), file_name);
}

void LoadEditorStateFromIniFile(EditorContext* const editor, const char* const file_name)
{
    size_t data_size = 0u;
    char* file_data = (char*)ImFileLoadToMemory(file_name, "rb", &data_size);

    if (!file_data)
    {
        return;
    }

    LoadEditorStateFromIniString(editor, file_data, data_size);
    ImGui::MemFree(file_data);
}
} // namespace imnodes
