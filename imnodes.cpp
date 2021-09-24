// the structure of this file:
//
// [SECTION] bezier curve helpers
// [SECTION] draw list helper
// [SECTION] ui state logic
// [SECTION] render helpers
// [SECTION] API implementation
// [SECTION] object helpers

#include "imnodes.h"
#include "imnodes_internal.h"

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

ImNodesContext* GImNodes = NULL;

namespace IMNODES_NAMESPACE
{
namespace
{
// [SECTION] bezier curve helpers

inline ImVec2 EvalCubicBezier(
    const float   t,
    const ImVec2& P0,
    const ImVec2& P1,
    const ImVec2& P2,
    const ImVec2& P3)
{
    // B(t) = (1-t)**3 p0 + 3(1 - t)**2 t P1 + 3(1-t)t**2 P2 + t**3 P3

    const float u = 1.0f - t;
    const float b0 = u * u * u;
    const float b1 = 3 * u * u * t;
    const float b2 = 3 * u * t * t;
    const float b3 = t * t * t;
    return ImVec2(
        b0 * P0.x + b1 * P1.x + b2 * P2.x + b3 * P3.x,
        b0 * P0.y + b1 * P1.y + b2 * P2.y + b3 * P3.y);
}

// Calculates the closest point along each bezier curve segment.
ImVec2 GetClosestPointOnCubicBezier(
    const int            num_segments,
    const ImVec2&        p,
    const ImCubicBezier& cb)
{
    IM_ASSERT(num_segments > 0);
    ImVec2 p_last = cb.P0;
    ImVec2 p_closest;
    float  p_closest_dist = FLT_MAX;
    float  t_step = 1.0f / (float)num_segments;
    for (int i = 1; i <= num_segments; ++i)
    {
        ImVec2 p_current = EvalCubicBezier(t_step * i, cb.P0, cb.P1, cb.P2, cb.P3);
        ImVec2 p_line = ImLineClosestPoint(p_last, p_current, p);
        float  dist = ImLengthSqr(p - p_line);
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
    const ImVec2&        pos,
    const ImCubicBezier& cubic_bezier,
    const int            num_segments)
{
    const ImVec2 point_on_curve = GetClosestPointOnCubicBezier(num_segments, pos, cubic_bezier);

    const ImVec2 to_curve = point_on_curve - pos;
    return ImSqrt(ImLengthSqr(to_curve));
}

inline ImRect GetContainingRectForCubicBezier(const ImCubicBezier& cb)
{
    const ImVec2 min = ImVec2(ImMin(cb.P0.x, cb.P3.x), ImMin(cb.P0.y, cb.P3.y));
    const ImVec2 max = ImVec2(ImMax(cb.P0.x, cb.P3.x), ImMax(cb.P0.y, cb.P3.y));

    const float hover_distance = GImNodes->Style.LinkHoverDistance;

    ImRect rect(min, max);
    rect.Add(cb.P1);
    rect.Add(cb.P2);
    rect.Expand(ImVec2(hover_distance, hover_distance));

    return rect;
}

inline ImCubicBezier CalcCubicBezier(
    ImVec2                     start,
    ImVec2                     end,
    const ImNodesAttributeType start_type,
    const float                line_segments_per_length)
{
    assert(
        (start_type == ImNodesAttributeType_Input) || (start_type == ImNodesAttributeType_Output));
    if (start_type == ImNodesAttributeType_Input)
    {
        ImSwap(start, end);
    }

    const float   link_length = ImSqrt(ImLengthSqr(end - start));
    const ImVec2  offset = ImVec2(0.25f * link_length, 0.f);
    ImCubicBezier cubic_bezier;
    cubic_bezier.P0 = start;
    cubic_bezier.P1 = start + offset;
    cubic_bezier.P2 = end - offset;
    cubic_bezier.P3 = end;
    cubic_bezier.NumSegments = ImMax(static_cast<int>(link_length * line_segments_per_length), 1);
    return cubic_bezier;
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

inline bool RectangleOverlapsBezier(const ImRect& rectangle, const ImCubicBezier& cubic_bezier)
{
    ImVec2 current =
        EvalCubicBezier(0.f, cubic_bezier.P0, cubic_bezier.P1, cubic_bezier.P2, cubic_bezier.P3);
    const float dt = 1.0f / cubic_bezier.NumSegments;
    for (int s = 0; s < cubic_bezier.NumSegments; ++s)
    {
        ImVec2 next = EvalCubicBezier(
            static_cast<float>((s + 1) * dt),
            cubic_bezier.P0,
            cubic_bezier.P1,
            cubic_bezier.P2,
            cubic_bezier.P3);
        if (RectangleOverlapsLineSegment(rectangle, current, next))
        {
            return true;
        }
        current = next;
    }
    return false;
}

inline bool RectangleOverlapsLink(const ImRect& rectangle, const ImCubicBezier& cubic_bezier)
{
    // First level: simple rejection test via rectangle overlap:

    const ImVec2& start = cubic_bezier.P0;
    const ImVec2& end = cubic_bezier.P3;

    ImRect containing_rect = ImRect(start, end);
    if (containing_rect.Min.x > containing_rect.Max.x)
    {
        ImSwap(containing_rect.Min.x, containing_rect.Max.x);
    }

    if (containing_rect.Min.y > containing_rect.Max.y)
    {
        ImSwap(containing_rect.Min.y, containing_rect.Max.y);
    }

    if (rectangle.Overlaps(containing_rect))
    {
        // First, check if either one or both endpoinds are trivially contained
        // in the rectangle

        if (rectangle.Contains(start) || rectangle.Contains(end))
        {
            return true;
        }

        // Second level of refinement: do a more expensive test against the
        // link

        return RectangleOverlapsBezier(rectangle, cubic_bezier);
    }

    return false;
}

// [SECTION] coordinate space conversion helpers

inline ImVec2 ScreenSpaceToGridSpace(const ImNodesEditorContext& editor, const ImVec2& v)
{
    return v - GImNodes->CanvasOriginScreenSpace - editor.Panning;
}

inline ImRect ScreenSpaceToGridSpace(const ImNodesEditorContext& editor, const ImRect& r)
{
    return ImRect(ScreenSpaceToGridSpace(editor, r.Min), ScreenSpaceToGridSpace(editor, r.Max));
}

inline ImVec2 GridSpaceToScreenSpace(const ImNodesEditorContext& editor, const ImVec2& v)
{
    return v + GImNodes->CanvasOriginScreenSpace + editor.Panning;
}

inline ImVec2 GridSpaceToCanvasSpace(const ImNodesEditorContext& editor, const ImVec2& v)
{
    return v + editor.Panning;
}

inline ImVec2 CanvasSpaceToGridSpace(const ImNodesEditorContext& editor, const ImVec2& v)
{
    return v - editor.Panning;
}

inline ImVec2 CanvasSpaceToScreenSpace(const ImVec2& v)
{
    return GImNodes->CanvasOriginScreenSpace + v;
}

inline ImVec2 MiniMapSpaceToGridSpace(const ImNodesEditorContext& editor, const ImVec2& v)
{
    return (v - editor.MiniMapContentScreenSpace.Min) / editor.MiniMapScaling +
           editor.GridContentBounds.Min;
};

inline ImVec2 ScreenSpaceToMiniMapSpace(const ImNodesEditorContext& editor, const ImVec2& v)
{
    return (ScreenSpaceToGridSpace(editor, v) - editor.GridContentBounds.Min) *
               editor.MiniMapScaling +
           editor.MiniMapContentScreenSpace.Min;
};

inline ImRect ScreenSpaceToMiniMapSpace(const ImNodesEditorContext& editor, const ImRect& r)
{
    return ImRect(
        ScreenSpaceToMiniMapSpace(editor, r.Min), ScreenSpaceToMiniMapSpace(editor, r.Max));
};

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

void DrawListSet(ImDrawList* window_draw_list) { GImNodes->CanvasDrawList = window_draw_list; }

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
//            |                     |                     |
//            |   submission idx    |...                  |
//            |                     |                     |
//            ---------------------------------------------

void DrawListAppendNodeChannels() { ImDrawListGrowChannels(GImNodes->CanvasDrawList, 2); }

void DrawListAppendClickInteractionChannel()
{
    // NOTE: don't use this function outside of EndNodeEditor. Using this before all nodes have been
    // added will screw up the node draw order.
    ImDrawListGrowChannels(GImNodes->CanvasDrawList, 1);
}

int DrawListSubmissionIdxToBackgroundChannelIdx(const int submission_idx)
{
    // NOTE: the first channel is the canvas background
    return 1 + 2 * submission_idx;
}

int DrawListSubmissionIdxToForegroundChannelIdx(const int submission_idx)
{
    return DrawListSubmissionIdxToBackgroundChannelIdx(submission_idx) + 1;
}

void DrawListActivateClickInteractionChannel()
{
    GImNodes->CanvasDrawList->_Splitter.SetCurrentChannel(
        GImNodes->CanvasDrawList, GImNodes->CanvasDrawList->_Splitter._Count - 1);
}

void DrawListActivateNodeBackground(const int node_submission_idx)
{
    const int background_channel_idx =
        DrawListSubmissionIdxToBackgroundChannelIdx(node_submission_idx);
    GImNodes->CanvasDrawList->_Splitter.SetCurrentChannel(
        GImNodes->CanvasDrawList, background_channel_idx);
}

void DrawListActivateNodeForeground(const int node_submission_idx)
{
    const int foreground_channel_idx =
        DrawListSubmissionIdxToForegroundChannelIdx(node_submission_idx);
    GImNodes->CanvasDrawList->_Splitter.SetCurrentChannel(
        GImNodes->CanvasDrawList, foreground_channel_idx);
}

// [SECTION] ui state logic

ImVec2 GetScreenSpacePinCoordinates(
    const ImRect&              node_rect,
    const ImRect&              attribute_rect,
    const ImNodesAttributeType type)
{
    assert(type == ImNodesAttributeType_Input || type == ImNodesAttributeType_Output);
    const float x = type == ImNodesAttributeType_Input
                        ? (node_rect.Min.x - GImNodes->Style.PinOffset)
                        : (node_rect.Max.x + GImNodes->Style.PinOffset);
    return ImVec2(x, 0.5f * (attribute_rect.Min.y + attribute_rect.Max.y));
}

bool MouseInCanvas()
{
    // This flag should be true either when hovering or clicking something in the canvas.
    const bool is_window_hovered_or_focused = ImGui::IsWindowHovered() || ImGui::IsWindowFocused();

    return is_window_hovered_or_focused &&
           GImNodes->CanvasRectScreenSpace.Contains(ImGui::GetMousePos());
}

void BeginNodeInteraction(ImNodesEditorContext& editor, const int node_id)
{
    // Don't start selecting a node if we are e.g. already creating and dragging
    // a new link! New link creation can happen when the mouse is clicked over
    // a node, but within the hover radius of a pin.
    if (editor.ClickInteraction.Type != ImNodesClickInteractionType_None)
    {
        return;
    }

    editor.ClickInteraction.Type = ImNodesClickInteractionType_Node;

    if (!editor.SelectedNodeIds.contains(node_id))
    {
        editor.SelectedNodeIds.resize(0);
        editor.SelectedLinkIds.resize(0);
        editor.SelectedNodeIds.push_back(node_id);
    }
}

void BeginLinkSelection(ImNodesEditorContext& editor, const int link_idx)
{
    editor.ClickInteraction.Type = ImNodesClickInteractionType_Link;
    // When a link is selected, clear all other selections, and insert the link
    // as the sole selection.
    editor.SelectedNodeIds.resize(0);
    editor.SelectedLinkIds.resize(0);
    editor.SelectedLinkIds.push_back(link_idx);
}

void BeginLinkDetach(ImNodesEditorContext& editor, const int link_idx, const int detached_at_pin_id)
{
    const ImLinkData& link = GImNodes->Links.Data[link_idx];
    GImNodes->DeletedLinkIdx = link_idx; // TODO: UI event for this?
    const int start_pin_id =
        detached_at_pin_id == link.StartPinId ? link.EndPinId : link.StartPinId;
    editor.ClickInteraction.StartUnconnectedLink(start_pin_id, ImNodesLinkCreatedFrom_Detach);
}

void BeginLinkInteraction(ImNodesEditorContext& editor, const int link_idx)
{
    // Check the 'click and drag to detach' case.
    if (GImNodes->HoveredPinIdx.HasValue())
    {
        const ImPinData& pin = GImNodes->Pins[GImNodes->HoveredPinIdx.Value()];
        if ((pin.Flags & ImNodesAttributeFlags_EnableLinkDetachWithDragClick) != 0)
        {
            BeginLinkDetach(editor, link_idx, pin.Id);
        }
    }
    // If we aren't near a pin, check if we are clicking the link with the modifier pressed. This
    // may also result in a link detach via clicking.
    else
    {
        const bool modifier_pressed = GImNodes->Io.LinkDetachWithModifierClick.Modifier == NULL
                                          ? false
                                          : *GImNodes->Io.LinkDetachWithModifierClick.Modifier;

        if (modifier_pressed)
        {
            const ImLinkData& link = GImNodes->Links.Data[link_idx];
            assert(GImNodes->PinIdToPinIdx.find(link.StartPinId) != GImNodes->PinIdToPinIdx.end());
            assert(GImNodes->PinIdToPinIdx.find(link.EndPinId) != GImNodes->PinIdToPinIdx.end());
            const int        start_pin_idx = GImNodes->PinIdToPinIdx[link.StartPinId];
            const int        end_pin_idx = GImNodes->PinIdToPinIdx[link.EndPinId];
            const ImPinData& start_pin = GImNodes->Pins[start_pin_idx];
            const ImPinData& end_pin = GImNodes->Pins[end_pin_idx];
            const ImVec2&    mouse_pos = GImNodes->MousePos;
            const float      dist_to_start = ImLengthSqr(start_pin.ScreenSpacePosition - mouse_pos);
            const float      dist_to_end = ImLengthSqr(end_pin.ScreenSpacePosition - mouse_pos);
            const int        closest_pin_id =
                dist_to_start < dist_to_end ? link.StartPinId : link.EndPinId;

            BeginLinkDetach(editor, link_idx, closest_pin_id);
        }
        else
        {
            BeginLinkSelection(editor, link_idx);
        }
    }
}

void BeginLinkCreation(ImNodesEditorContext& editor, const int started_at_pin_id)
{
    editor.ClickInteraction.StartUnconnectedLink(started_at_pin_id, ImNodesLinkCreatedFrom_Pin);
    GImNodes->UIEvent.StartLink(started_at_pin_id);
}

static inline bool IsMiniMapHovered();

void BeginCanvasInteraction(ImNodesEditorContext& editor)
{
    const bool any_ui_element_hovered =
        GImNodes->HoveredNodeIdx.HasValue() || GImNodes->HoveredLinkIdx.HasValue() ||
        GImNodes->HoveredPinIdx.HasValue() || ImGui::IsAnyItemHovered();

    const bool mouse_not_in_canvas = !MouseInCanvas();

    if (editor.ClickInteraction.Type != ImNodesClickInteractionType_None ||
        any_ui_element_hovered || mouse_not_in_canvas)
    {
        return;
    }

    const bool started_panning = GImNodes->AltMouseClicked;

    if (started_panning)
    {
        editor.ClickInteraction.Type = ImNodesClickInteractionType_Panning;
    }
    else if (GImNodes->LeftMouseClicked)
    {
        editor.ClickInteraction.Type = ImNodesClickInteractionType_BoxSelection;
        editor.ClickInteraction.BoxSelector.Rectangle.Min =
            ScreenSpaceToGridSpace(editor, GImNodes->MousePos);
    }
}

void BoxSelectorUpdateSelection(ImNodesEditorContext& editor, ImRect box_rect)
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

    editor.SelectedNodeIds.resize(0);

    // Test for overlap against node rectangles

    for (int node_idx = 0; node_idx < GImNodes->Nodes.size(); ++node_idx)
    {
        ImNodeDrawData& node = GImNodes->Nodes[node_idx];
        if (box_rect.Overlaps(node.BaseRectangle))
        {
            editor.SelectedNodeIds.push_back(node.Id);
        }
    }

    // Update link selection

    editor.SelectedLinkIds.resize(0);

    // Test for overlap against links

    for (int link_idx = 0; link_idx < GImNodes->Links.size(); ++link_idx)
    {
        const ImCubicBezier& cubic_bezier = GImNodes->Links.CubicBeziers[link_idx];
        if (RectangleOverlapsLink(box_rect, cubic_bezier))
        {
            const int link_id = GImNodes->Links.Data[link_idx].LinkId;
            editor.SelectedLinkIds.push_back(link_id);
        }
    }
}

void TranslateSelectedNodes(ImNodesEditorContext& editor)
{
    const ImGuiIO&         io = ImGui::GetIO();
    std::map<int, ImVec2>& node_origins = editor.GridSpaceNodeOrigins;
    for (int i = 0; i < editor.SelectedNodeIds.size(); ++i)
    {
        const int                       node_id = editor.SelectedNodeIds[i];
        std::map<int, ImVec2>::iterator id_pos_pair = node_origins.find(node_id);
        assert(id_pos_pair != node_origins.end());
        id_pos_pair->second += io.MouseDelta - editor.AutoPanningDelta;
    }
}

ImOptionalIndex FindDuplicateLink(
    const ImVector<ImLinkData>& links,
    const int                   start_pin_id,
    const int                   end_pin_id)
{
    for (int idx = 0; idx < links.size(); ++idx)
    {
        const ImLinkData& link = links[idx];

        int lhs_start_id = start_pin_id;
        int lhs_end_id = end_pin_id;
        int rhs_start_id = link.StartPinId;
        int rhs_end_id = link.EndPinId;

        if (lhs_start_id > lhs_end_id)
        {
            ImSwap(lhs_start_id, lhs_end_id);
        }

        if (rhs_start_id > rhs_end_id)
        {
            ImSwap(rhs_start_id, rhs_end_id);
        }

        if (lhs_start_id == rhs_start_id && lhs_end_id == rhs_end_id)
        {
            return ImOptionalIndex(idx);
        }
    }

    return ImOptionalIndex();
}

bool ShouldLinkSnapToPin(
    const ImNodesEditorContext& editor,
    const ImPinData&            start_pin,
    const int                   hovered_pin_idx,
    const ImOptionalIndex       duplicate_link)
{
    const ImPinData& end_pin = GImNodes->Pins[hovered_pin_idx];

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
    if (duplicate_link.HasValue() && !(duplicate_link == GImNodes->SnapLinkIdx))
    {
        return false;
    }

    return true;
}

void ClickInteractionUpdate(ImNodesEditorContext& editor)
{
    switch (editor.ClickInteraction.Type)
    {
    case ImNodesClickInteractionType_BoxSelection:
    {
        editor.ClickInteraction.BoxSelector.Rectangle.Max =
            ScreenSpaceToGridSpace(editor, GImNodes->MousePos);

        ImRect box_rect = editor.ClickInteraction.BoxSelector.Rectangle;
        box_rect.Min = GridSpaceToScreenSpace(editor, box_rect.Min);
        box_rect.Max = GridSpaceToScreenSpace(editor, box_rect.Max);

        BoxSelectorUpdateSelection(editor, box_rect);

        const ImU32 box_selector_color = GImNodes->Style.Colors[ImNodesCol_BoxSelector];
        const ImU32 box_selector_outline = GImNodes->Style.Colors[ImNodesCol_BoxSelectorOutline];
        GImNodes->CanvasDrawList->AddRectFilled(box_rect.Min, box_rect.Max, box_selector_color);
        GImNodes->CanvasDrawList->AddRect(box_rect.Min, box_rect.Max, box_selector_outline);

        if (GImNodes->LeftMouseReleased)
        {
            editor.ClickInteraction.Type = ImNodesClickInteractionType_None;
        }
    }
    break;
    case ImNodesClickInteractionType_Node:
    {
        if (GImNodes->LeftMouseDragging)
        {
            TranslateSelectedNodes(editor);
        }
        else if (GImNodes->LeftMouseReleased)
        {
            editor.ClickInteraction.Type = ImNodesClickInteractionType_None;
        }
    }
    break;
    case ImNodesClickInteractionType_Link:
    {
        if (GImNodes->LeftMouseReleased)
        {
            editor.ClickInteraction.Type = ImNodesClickInteractionType_None;
        }
    }
    break;
    case ImNodesClickInteractionType_UnconnectedLink:
    {
        const int start_pin_id = editor.ClickInteraction.UnconnectedLink.StartPinId;
        assert(GImNodes->PinIdToPinIdx.find(start_pin_id) != GImNodes->PinIdToPinIdx.end());
        const int        start_pin_idx = GImNodes->PinIdToPinIdx[start_pin_id];
        const ImPinData& start_pin = GImNodes->Pins[start_pin_idx];

        const ImOptionalIndex maybe_duplicate_link_idx =
            GImNodes->HoveredPinIdx.HasValue()
                ? FindDuplicateLink(
                      GImNodes->Links.Data,
                      start_pin_id,
                      GImNodes->Pins[GImNodes->HoveredPinIdx.Value()].Id)
                : ImOptionalIndex();

        const bool should_snap =
            GImNodes->HoveredPinIdx.HasValue() &&
            ShouldLinkSnapToPin(
                editor, start_pin, GImNodes->HoveredPinIdx.Value(), maybe_duplicate_link_idx);

        const ImVec2& start_pos = start_pin.ScreenSpacePosition;

        // If we are within the hover radius of a receiving pin, snap the link endpoint to it
        ImVec2 end_pos;
        {
            if (should_snap)
            {
                const ImPinData& end_pin = GImNodes->Pins[GImNodes->HoveredPinIdx.Value()];
                end_pos = end_pin.ScreenSpacePosition;
            }
            else
            {
                end_pos = GImNodes->MousePos;
            }
        }

        const ImCubicBezier cubic_bezier = CalcCubicBezier(
            start_pos, end_pos, start_pin.Type, GImNodes->Style.LinkLineSegmentsPerLength);
#if IMGUI_VERSION_NUM < 18000
        GImNodes->CanvasDrawList->AddBezierCurve(
#else
        GImNodes->CanvasDrawList->AddBezierCubic(
#endif
            cubic_bezier.P0,
            cubic_bezier.P1,
            cubic_bezier.P2,
            cubic_bezier.P3,
            GImNodes->Style.Colors[ImNodesCol_Link],
            GImNodes->Style.LinkThickness,
            cubic_bezier.NumSegments);

        const bool link_creation_on_snap = GImNodes->HoveredPinIdx.HasValue() &&
                                           (GImNodes->Pins[GImNodes->HoveredPinIdx.Value()].Flags &
                                            ImNodesAttributeFlags_EnableLinkCreationOnSnap);

        if (GImNodes->LeftMouseReleased)
        {
            const ImNodesLinkCreatedFrom created_from_type =
                editor.ClickInteraction.UnconnectedLink.FromType;

            if (should_snap)
            {
                const int end_pin_id = GImNodes->Pins[GImNodes->HoveredPinIdx.Value()].Id;
                GImNodes->UIEvent.CreateLink(start_pin_id, end_pin_id, created_from_type);
            }
            else
            {
                GImNodes->UIEvent.DropLink(start_pin_id, created_from_type);
            }

            editor.ClickInteraction.Type = ImNodesClickInteractionType_None;
        }
        else if (should_snap && link_creation_on_snap)
        {
            const int snapped_pin_id = GImNodes->Pins[GImNodes->HoveredPinIdx.Value()].Id;
            GImNodes->UIEvent.CreateLink(
                start_pin_id, snapped_pin_id, editor.ClickInteraction.UnconnectedLink.FromType);
            editor.ClickInteraction.SnapUnconnectedLinkToPin(snapped_pin_id);
        }
    }
    break;
    case ImNodesClickInteractionType_SnappedLink:
    {
        // TODO: what if the pin id changes?
        const bool snapping_pin_changed = !GImNodes->HoveredPinIdx.HasValue();

        // Detach the link that was created by this link event if it's no longer in snap range
        if (snapping_pin_changed)
        {
            editor.ClickInteraction.UnsnapLinkFromPin();
            if (GImNodes->SnapLinkIdx.HasValue())
            {
                GImNodes->DeletedLinkIdx = GImNodes->SnapLinkIdx.Value();
            }
        }

        // TODO: what if SnapLinkIdx doesn't have a value? Render the snapped link here?

        if (GImNodes->LeftMouseReleased)
        {
            editor.ClickInteraction.Type = ImNodesClickInteractionType_None;
        }
    }
    break;
    case ImNodesClickInteractionType_Panning:
    {
        const bool dragging = GImNodes->AltMouseDragging;

        if (dragging)
        {
            editor.Panning += ImGui::GetIO().MouseDelta;
        }
        else
        {
            editor.ClickInteraction.Type = ImNodesClickInteractionType_None;
        }
    }
    break;
    case ImNodesClickInteractionType_ImGuiItem:
    {
        if (GImNodes->LeftMouseReleased)
        {
            editor.ClickInteraction.Type = ImNodesClickInteractionType_None;
        }
    }
    case ImNodesClickInteractionType_None:
        break;
    default:
        assert(!"Unreachable code!");
        break;
    }
}

ImOptionalIndex ResolveHoveredNode() { return GImNodes->NodeOverlappingCursor; }

ImOptionalIndex ResolveHoveredPin(const ImVector<ImPinData>& pins)
{
    float           smallest_distance = FLT_MAX;
    ImOptionalIndex pin_idx_with_smallest_distance;

    const float hover_radius_sqr = GImNodes->Style.PinHoverRadius * GImNodes->Style.PinHoverRadius;

    for (int idx = 0; idx < pins.size(); ++idx)
    {

        const ImVec2& pin_pos = GImNodes->Pins[idx].ScreenSpacePosition;
        const float   distance_sqr = ImLengthSqr(pin_pos - GImNodes->MousePos);

        // TODO: GImNodes->Style.PinHoverRadius needs to be copied into pin data and the pin-local
        // value used here. This is no longer called in BeginAttribute/EndAttribute scope and the
        // detected pin might have a different hover radius than what the user had when calling
        // BeginAttribute/EndAttribute.
        if (distance_sqr < hover_radius_sqr && distance_sqr < smallest_distance)
        {
            smallest_distance = distance_sqr;
            pin_idx_with_smallest_distance = idx;
        }
    }

    return pin_idx_with_smallest_distance;
}

ImOptionalIndex ResolveHoveredLink(const ImVector<ImPinData>& pins, const ImLinks& links)
{
    float           smallest_distance = FLT_MAX;
    ImOptionalIndex link_idx_with_smallest_distance;

    // There are two ways a link can be detected as "hovered".
    // 1. The link is within hover distance to the mouse. The closest such link is selected as being
    // hovered over.
    // 2. If the link is connected to the currently hovered pin.
    //
    // The latter is a requirement for link detaching with drag click to work, as both a link and
    // pin are required to be hovered over for the feature to work.

    assert(links.Data.size() == links.CubicBeziers.size());

    const bool is_pin_hovered = GImNodes->HoveredPinIdx.HasValue();
    const int  hovered_pin_id = is_pin_hovered ? pins[GImNodes->HoveredPinIdx.Value()].Id : 0;

    for (int idx = 0; idx < links.size(); ++idx)
    {
        const ImLinkData&    link = links.Data[idx];
        const ImCubicBezier& cubic_bezier = links.CubicBeziers[idx];

        if (is_pin_hovered &&
            (hovered_pin_id == link.StartPinId || hovered_pin_id == link.EndPinId))
        {
            return idx;
        }

        // The distance test
        {
            const ImRect curve_bounds = GetContainingRectForCubicBezier(cubic_bezier);

            // First, do a simple bounding box test against the box containing the link
            // to see whether calculating the distance to the link is worth doing.
            if (curve_bounds.Contains(GImNodes->MousePos))
            {
                const float distance = GetDistanceToCubicBezier(
                    GImNodes->MousePos, cubic_bezier, cubic_bezier.NumSegments);

                // TODO: GImNodes->Style.LinkHoverDistance could be also copied into ImLinkData,
                // since we're not calling this function in the same scope as ImNodes::Link(). The
                // rendered/detected link might have a different hover distance than what the user
                // had specified when calling Link()
                if (distance < GImNodes->Style.LinkHoverDistance && distance < smallest_distance)
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

inline ImRect GetNodeTitleRect(const ImNodeDrawData& node)
{
    ImRect expanded_title_rect = node.TitleRectangle;
    expanded_title_rect.Expand(node.LayoutStyle.Padding);

    return ImRect(
        expanded_title_rect.Min,
        expanded_title_rect.Min + ImVec2(node.BaseRectangle.GetWidth(), 0.f) +
            ImVec2(0.f, expanded_title_rect.GetHeight()));
}

void DrawGrid(ImNodesEditorContext& editor, const ImVec2& canvas_size)
{
    const ImVec2 offset = editor.Panning;

    for (float x = fmodf(offset.x, GImNodes->Style.GridSpacing); x < canvas_size.x;
         x += GImNodes->Style.GridSpacing)
    {
        GImNodes->CanvasDrawList->AddLine(
            CanvasSpaceToScreenSpace(ImVec2(x, 0.0f)),
            CanvasSpaceToScreenSpace(ImVec2(x, canvas_size.y)),
            GImNodes->Style.Colors[ImNodesCol_GridLine]);
    }

    for (float y = fmodf(offset.y, GImNodes->Style.GridSpacing); y < canvas_size.y;
         y += GImNodes->Style.GridSpacing)
    {
        GImNodes->CanvasDrawList->AddLine(
            CanvasSpaceToScreenSpace(ImVec2(0.0f, y)),
            CanvasSpaceToScreenSpace(ImVec2(canvas_size.x, y)),
            GImNodes->Style.Colors[ImNodesCol_GridLine]);
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

void DrawPinShape(const ImVec2& pin_pos, const ImPinData& pin, const ImU32 pin_color)
{
    static const int CIRCLE_NUM_SEGMENTS = 8;

    switch (pin.Shape)
    {
    case ImNodesPinShape_Circle:
    {
        GImNodes->CanvasDrawList->AddCircle(
            pin_pos,
            GImNodes->Style.PinCircleRadius,
            pin_color,
            CIRCLE_NUM_SEGMENTS,
            GImNodes->Style.PinLineThickness);
    }
    break;
    case ImNodesPinShape_CircleFilled:
    {
        GImNodes->CanvasDrawList->AddCircleFilled(
            pin_pos, GImNodes->Style.PinCircleRadius, pin_color, CIRCLE_NUM_SEGMENTS);
    }
    break;
    case ImNodesPinShape_Quad:
    {
        const QuadOffsets offset = CalculateQuadOffsets(GImNodes->Style.PinQuadSideLength);
        GImNodes->CanvasDrawList->AddQuad(
            pin_pos + offset.TopLeft,
            pin_pos + offset.BottomLeft,
            pin_pos + offset.BottomRight,
            pin_pos + offset.TopRight,
            pin_color,
            GImNodes->Style.PinLineThickness);
    }
    break;
    case ImNodesPinShape_QuadFilled:
    {
        const QuadOffsets offset = CalculateQuadOffsets(GImNodes->Style.PinQuadSideLength);
        GImNodes->CanvasDrawList->AddQuadFilled(
            pin_pos + offset.TopLeft,
            pin_pos + offset.BottomLeft,
            pin_pos + offset.BottomRight,
            pin_pos + offset.TopRight,
            pin_color);
    }
    break;
    case ImNodesPinShape_Triangle:
    {
        const TriangleOffsets offset =
            CalculateTriangleOffsets(GImNodes->Style.PinTriangleSideLength);
        GImNodes->CanvasDrawList->AddTriangle(
            pin_pos + offset.TopLeft,
            pin_pos + offset.BottomLeft,
            pin_pos + offset.Right,
            pin_color,
            // NOTE: for some weird reason, the line drawn by AddTriangle is
            // much thinner than the lines drawn by AddCircle or AddQuad.
            // Multiplying the line thickness by two seemed to solve the
            // problem at a few different thickness values.
            2.f * GImNodes->Style.PinLineThickness);
    }
    break;
    case ImNodesPinShape_TriangleFilled:
    {
        const TriangleOffsets offset =
            CalculateTriangleOffsets(GImNodes->Style.PinTriangleSideLength);
        GImNodes->CanvasDrawList->AddTriangleFilled(
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

void DrawNodesAndPins(
    const ImNodesEditorContext& editor,
    const ImOptionalIndex       maybe_hovered_node_idx,
    const ImOptionalIndex       maybe_hovered_pin_idx)
{
    assert(GImNodes->Nodes.size() == GImNodes->NodeIdxToPinIndices.size());

    for (int node_idx = 0; node_idx < GImNodes->Nodes.size(); ++node_idx)
    {
        DrawListActivateNodeBackground(node_idx);

        // Submit the node rectangle draw commands

        const ImNodeDrawData& node = GImNodes->Nodes[node_idx];

        ImU32 node_background_color = node.ColorStyle.Background;
        ImU32 titlebar_background_color = node.ColorStyle.Titlebar;

        if (editor.SelectedNodeIds.contains(node.Id))
        {
            node_background_color = node.ColorStyle.BackgroundSelected;
            titlebar_background_color = node.ColorStyle.TitlebarSelected;
        }
        else if (maybe_hovered_node_idx == node_idx)
        {
            node_background_color = node.ColorStyle.BackgroundHovered;
            titlebar_background_color = node.ColorStyle.TitlebarHovered;
        }

        GImNodes->CanvasDrawList->AddRectFilled(
            node.BaseRectangle.Min,
            node.BaseRectangle.Max,
            node_background_color,
            node.LayoutStyle.CornerRounding);

        if (node.TitleRectangle.GetHeight() > 0.f)
        {
            ImRect titlebar_rect = node.TitleRectangle;
            titlebar_rect.Expand(node.LayoutStyle.Padding);
            titlebar_rect.Max.x = node.BaseRectangle.Max.x;

#if IMGUI_VERSION_NUM < 18200
            GImNodes->CanvasDrawList->AddRectFilled(
                titlebar_rect.Min,
                titlebar_rect.Max,
                titlebar_background_color,
                node.LayoutStyle.CornerRounding,
                ImDrawCornerFlags_Top);
#else
            GImNodes->CanvasDrawList->AddRectFilled(
                titlebar_rect.Min,
                titlebar_rect.Max,
                titlebar_background_color,
                node.LayoutStyle.CornerRounding,
                ImDrawFlags_RoundCornersTop);
#endif
        }

        // Submit the pin draw commands

        for (int i = 0; i < GImNodes->NodeIdxToPinIndices[node_idx].size(); ++i)
        {
            const int        pin_idx = GImNodes->NodeIdxToPinIndices[node_idx][i];
            const ImPinData& pin = GImNodes->Pins[pin_idx];

            ImU32 pin_color = pin.ColorStyle.Background;

            if (maybe_hovered_pin_idx == pin_idx)
            {
                pin_color = pin.ColorStyle.Hovered;
            }

            DrawPinShape(pin.ScreenSpacePosition, pin, pin_color);
        }
    }
}

void CalcLinkGeometries(const ImVector<ImPinData>& pins, ImLinks& links)
{
    assert(links.CubicBeziers.empty());

    const ImVector<ImLinkData>& link_data = links.Data;
    ImVector<ImCubicBezier>&    cubic_beziers = links.CubicBeziers;

    for (int idx = 0; idx < link_data.size(); ++idx)
    {
        const ImLinkData& link = link_data[idx];

        assert(GImNodes->PinIdToPinIdx.find(link.StartPinId) != GImNodes->PinIdToPinIdx.end());
        assert(GImNodes->PinIdToPinIdx.find(link.EndPinId) != GImNodes->PinIdToPinIdx.end());

        const int        start_pin_idx = GImNodes->PinIdToPinIdx[link.StartPinId];
        const int        end_pin_idx = GImNodes->PinIdToPinIdx[link.EndPinId];
        const ImPinData& start_pin = pins[start_pin_idx];
        const ImPinData& end_pin = pins[end_pin_idx];

        cubic_beziers.push_back(CalcCubicBezier(
            start_pin.ScreenSpacePosition,
            end_pin.ScreenSpacePosition,
            start_pin.Type,
            GImNodes->Style.LinkLineSegmentsPerLength));
    }
}

void DrawLinks(const ImNodesEditorContext& editor, const ImLinks& links)
{
    assert(links.Data.size() == links.CubicBeziers.size());

    for (int link_idx = 0; link_idx < links.size(); ++link_idx)
    {
        const ImLinkData&    data = links.Data[link_idx];
        const ImCubicBezier& cb = links.CubicBeziers[link_idx];

        const bool link_hovered = GImNodes->HoveredLinkIdx == link_idx;

        // TODO: DeletedLinkIdx
        // Do we really need to skip rendering links if the user hasn't deleted the link yet?

        ImU32 link_color = data.BaseColor;
        if (link_hovered)
        {
            link_color = data.HoveredColor;
        }
        else if (editor.SelectedLinkIds.contains(data.LinkId))
        {
            link_color = data.SelectedColor;
        }

#if IMGUI_VERSION_NUM < 18000
        GImNodes->CanvasDrawList->AddBezierCurve(
#else
        GImNodes->CanvasDrawList->AddBezierCubic(
#endif
            cb.P0, cb.P1, cb.P2, cb.P3, link_color, GImNodes->Style.LinkThickness, cb.NumSegments);
    }
}

void BeginPinAttribute(
    const int                  id,
    const ImNodesAttributeType type,
    const ImNodesPinShape      shape,
    const int                  node_idx)
{
    // Make sure to call BeginNode() before calling
    // BeginAttribute()
    assert(GImNodes->CurrentScope == ImNodesScope_Node);
    GImNodes->CurrentScope = ImNodesScope_Attribute;

    ImGui::BeginGroup();
    ImGui::PushID(id);

    // TODO: is this actually used for anything?
    GImNodes->CurrentAttributeId = id;

    {
        const int pin_idx = GImNodes->Pins.size();
        GImNodes->PinIdToPinIdx.insert(std::make_pair(id, pin_idx));
        GImNodes->NodeIdxToPinIndices.back().push_back(pin_idx);
    }

    ImPinData pin(id);
    pin.ParentNodeIdx = node_idx;
    pin.Type = type;
    pin.Shape = shape;
    pin.Flags = GImNodes->CurrentAttributeFlags;
    pin.ColorStyle.Background = GImNodes->Style.Colors[ImNodesCol_Pin];
    pin.ColorStyle.Hovered = GImNodes->Style.Colors[ImNodesCol_PinHovered];
    GImNodes->Pins.push_back(pin);
}

void EndPinAttribute()
{
    assert(GImNodes->CurrentScope == ImNodesScope_Attribute);
    GImNodes->CurrentScope = ImNodesScope_Node;

    ImGui::PopID();
    ImGui::EndGroup();

    if (ImGui::IsItemActive())
    {
        GImNodes->ActiveAttribute = true;
        GImNodes->ActiveAttributeId = GImNodes->CurrentAttributeId;
    }

    GImNodes->PinAttributeRectangles.push_back(
        ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()));
}

void Initialize(ImNodesContext* context)
{
    context->CanvasOriginScreenSpace = ImVec2(0.0f, 0.0f);
    context->CanvasRectScreenSpace = ImRect(ImVec2(0.f, 0.f), ImVec2(0.f, 0.f));
    context->CurrentScope = ImNodesScope_None;

    context->CurrentNodeIdx = INT_MAX;

    context->DefaultEditorCtx = EditorContextCreate();
    EditorContextSet(GImNodes->DefaultEditorCtx);

    context->CurrentAttributeFlags = ImNodesAttributeFlags_None;
    context->AttributeFlagStack.push_back(GImNodes->CurrentAttributeFlags);

    StyleColorsDark();
}

void Shutdown(ImNodesContext* ctx) { EditorContextFree(ctx->DefaultEditorCtx); }

// [SECTION] minimap

static inline bool IsMiniMapActive()
{
    ImNodesEditorContext& editor = EditorContextGet();
    return editor.MiniMapEnabled && editor.MiniMapSizeFraction > 0.0f;
}

static inline bool IsMiniMapHovered()
{
    ImNodesEditorContext& editor = EditorContextGet();
    return IsMiniMapActive() &&
           ImGui::IsMouseHoveringRect(
               editor.MiniMapRectScreenSpace.Min, editor.MiniMapRectScreenSpace.Max);
}

static inline void CalcMiniMapLayout()
{
    ImNodesEditorContext& editor = EditorContextGet();
    const ImVec2          offset = GImNodes->Style.MiniMapOffset;
    const ImVec2          border = GImNodes->Style.MiniMapPadding;
    const ImRect          editor_rect = GImNodes->CanvasRectScreenSpace;

    // Compute the size of the mini-map area
    ImVec2 mini_map_size;
    float  mini_map_scaling;
    {
        const ImVec2 max_size =
            ImFloor(editor_rect.GetSize() * editor.MiniMapSizeFraction - border * 2.0f);
        const float  max_size_aspect_ratio = max_size.x / max_size.y;
        const ImVec2 grid_content_size = editor.GridContentBounds.IsInverted()
                                             ? max_size
                                             : ImFloor(editor.GridContentBounds.GetSize());
        const float  grid_content_aspect_ratio = grid_content_size.x / grid_content_size.y;
        mini_map_size = ImFloor(
            grid_content_aspect_ratio > max_size_aspect_ratio
                ? ImVec2(max_size.x, max_size.x / grid_content_aspect_ratio)
                : ImVec2(max_size.y * grid_content_aspect_ratio, max_size.y));
        mini_map_scaling = mini_map_size.x / grid_content_size.x;
    }

    // Compute location of the mini-map
    ImVec2 mini_map_pos;
    {
        ImVec2 align;

        switch (editor.MiniMapLocation)
        {
        case ImNodesMiniMapLocation_BottomRight:
            align.x = 1.0f;
            align.y = 1.0f;
            break;
        case ImNodesMiniMapLocation_BottomLeft:
            align.x = 0.0f;
            align.y = 1.0f;
            break;
        case ImNodesMiniMapLocation_TopRight:
            align.x = 1.0f;
            align.y = 0.0f;
            break;
        case ImNodesMiniMapLocation_TopLeft: // [[fallthrough]]
        default:
            align.x = 0.0f;
            align.y = 0.0f;
            break;
        }

        const ImVec2 top_left_pos = editor_rect.Min + offset + border;
        const ImVec2 bottom_right_pos = editor_rect.Max - offset - border - mini_map_size;
        mini_map_pos = ImFloor(ImLerp(top_left_pos, bottom_right_pos, align));
    }

    editor.MiniMapRectScreenSpace =
        ImRect(mini_map_pos - border, mini_map_pos + mini_map_size + border);
    editor.MiniMapContentScreenSpace = ImRect(mini_map_pos, mini_map_pos + mini_map_size);
    editor.MiniMapScaling = mini_map_scaling;
}

static void MiniMapDrawNodes(const ImNodesEditorContext& editor)
{
    for (int node_idx = 0; node_idx < GImNodes->Nodes.size(); ++node_idx)
    {
        const ImNodeDrawData& node = GImNodes->Nodes[node_idx];

        const ImRect node_rect = ScreenSpaceToMiniMapSpace(editor, node.BaseRectangle);

        ImU32 mini_map_node_background;

        if (editor.ClickInteraction.Type == ImNodesClickInteractionType_None &&
            ImGui::IsMouseHoveringRect(node_rect.Min, node_rect.Max))
        {
            mini_map_node_background =
                GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundHovered];

            // Run user callback when hovering a mini-map node
            if (editor.MiniMapNodeHoveringCallback)
            {
                editor.MiniMapNodeHoveringCallback(
                    node.Id, editor.MiniMapNodeHoveringCallbackUserData);
            }
        }
        else if (editor.SelectedNodeIds.contains(node.Id))
        {
            mini_map_node_background =
                GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundSelected];
        }
        else
        {
            mini_map_node_background = GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackground];
        }

        // Round to near whole pixel value for corner-rounding to prevent visual glitches
        const float mini_map_node_rounding =
            floorf(node.LayoutStyle.CornerRounding * editor.MiniMapScaling);

        const ImU32 mini_map_node_outline = GImNodes->Style.Colors[ImNodesCol_MiniMapNodeOutline];

        GImNodes->CanvasDrawList->AddRectFilled(
            node_rect.Min, node_rect.Max, mini_map_node_background, mini_map_node_rounding);

        GImNodes->CanvasDrawList->AddRect(
            node_rect.Min, node_rect.Max, mini_map_node_outline, mini_map_node_rounding);
    }
}

static void MiniMapDrawLinks(const ImNodesEditorContext& editor, const ImLinks& links)
{
    assert(links.Data.size() == links.CubicBeziers.size());

    for (int idx = 0; idx < links.size(); ++idx)
    {
        const int            link_id = links.Data[idx].LinkId;
        const ImCubicBezier& cb = links.CubicBeziers[idx];

        // TODO: handle DeletedLinkIdx?

        const ImU32 link_color =
            GImNodes->Style.Colors
                [editor.SelectedLinkIds.contains(link_id) ? ImNodesCol_MiniMapLinkSelected
                                                          : ImNodesCol_MiniMapLink];

#if IMGUI_VERSION_NUM < 18000
        GImNodes->CanvasDrawList->AddBezierCurve(
#else
        GImNodes->CanvasDrawList->AddBezierCubic(
#endif
            ScreenSpaceToMiniMapSpace(editor, cb.P0),
            ScreenSpaceToMiniMapSpace(editor, cb.P1),
            ScreenSpaceToMiniMapSpace(editor, cb.P2),
            ScreenSpaceToMiniMapSpace(editor, cb.P3),
            link_color,
            GImNodes->Style.LinkThickness * editor.MiniMapScaling,
            cb.NumSegments);
    }
}

static void MiniMapUpdate()
{
    ImNodesEditorContext& editor = EditorContextGet();

    ImU32 mini_map_background;

    if (IsMiniMapHovered())
    {
        mini_map_background = GImNodes->Style.Colors[ImNodesCol_MiniMapBackgroundHovered];
    }
    else
    {
        mini_map_background = GImNodes->Style.Colors[ImNodesCol_MiniMapBackground];
    }

    // Create a child window bellow mini-map, so it blocks all mouse interaction on canvas.
    int flags = ImGuiWindowFlags_NoBackground;
    ImGui::SetCursorScreenPos(editor.MiniMapRectScreenSpace.Min);
    ImGui::BeginChild("minimap", editor.MiniMapRectScreenSpace.GetSize(), false, flags);

    const ImRect& mini_map_rect = editor.MiniMapRectScreenSpace;

    // Draw minimap background and border
    GImNodes->CanvasDrawList->AddRectFilled(
        mini_map_rect.Min, mini_map_rect.Max, mini_map_background);

    GImNodes->CanvasDrawList->AddRect(
        mini_map_rect.Min, mini_map_rect.Max, GImNodes->Style.Colors[ImNodesCol_MiniMapOutline]);

    // Clip draw list items to mini-map rect (after drawing background/outline)
    GImNodes->CanvasDrawList->PushClipRect(
        mini_map_rect.Min, mini_map_rect.Max, true /* intersect with editor clip-rect */);

    // Draw links first so they appear under nodes, and we can use the same draw channel

    MiniMapDrawLinks(editor, GImNodes->Links);

    MiniMapDrawNodes(editor);

    // Draw editor canvas rect inside mini-map
    {
        const ImU32  canvas_color = GImNodes->Style.Colors[ImNodesCol_MiniMapCanvas];
        const ImU32  outline_color = GImNodes->Style.Colors[ImNodesCol_MiniMapCanvasOutline];
        const ImRect rect = ScreenSpaceToMiniMapSpace(editor, GImNodes->CanvasRectScreenSpace);

        GImNodes->CanvasDrawList->AddRectFilled(rect.Min, rect.Max, canvas_color);
        GImNodes->CanvasDrawList->AddRect(rect.Min, rect.Max, outline_color);
    }

    // Have to pop mini-map clip rect
    GImNodes->CanvasDrawList->PopClipRect();

    bool mini_map_is_hovered = ImGui::IsWindowHovered();

    ImGui::EndChild();

    bool center_on_click = mini_map_is_hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
                           editor.ClickInteraction.Type == ImNodesClickInteractionType_None &&
                           !GImNodes->Nodes.empty();
    if (center_on_click)
    {
        ImVec2 target = MiniMapSpaceToGridSpace(editor, ImGui::GetMousePos());
        ImVec2 center = GImNodes->CanvasRectScreenSpace.GetSize() * 0.5f;
        editor.Panning = ImFloor(center - target);
    }

    // Reset callback info after use
    editor.MiniMapNodeHoveringCallback = NULL;
    editor.MiniMapNodeHoveringCallbackUserData = NULL;
}

// [SECTION] object helpers

const ImNodeDrawData& FindNode(const ImVector<ImNodeDrawData>& nodes, const int node_id)
{
    const ImNodeDrawData*       data = nodes.begin();
    const ImNodeDrawData* const data_end = nodes.end();
    while (data < data_end)
    {
        if (data->Id == node_id)
        {
            break;
        }
        else
        {
            ++data;
        }
    }

    // Post-condition: a node with the given node id has to exist.
    assert(data != data_end);

    return *data;
}

ImNodeDrawData& FindNode(ImVector<ImNodeDrawData>& nodes, const int node_id)
{
    return const_cast<ImNodeDrawData&>(
        FindNode(const_cast<const ImVector<ImNodeDrawData>&>(nodes), node_id));
}

} // namespace
} // namespace IMNODES_NAMESPACE

// [SECTION] API implementation

ImNodesIO::EmulateThreeButtonMouse::EmulateThreeButtonMouse() : Modifier(NULL) {}

ImNodesIO::LinkDetachWithModifierClick::LinkDetachWithModifierClick() : Modifier(NULL) {}

ImNodesIO::ImNodesIO()
    : EmulateThreeButtonMouse(), LinkDetachWithModifierClick(),
      AltMouseButton(ImGuiMouseButton_Middle), AutoPanningSpeed(1000.0f)
{
}

ImNodesStyle::ImNodesStyle()
    : GridSpacing(32.f), NodeCornerRounding(4.f), NodePadding(8.f, 8.f), NodeBorderThickness(1.f),
      LinkThickness(3.f), LinkLineSegmentsPerLength(0.1f), LinkHoverDistance(10.f),
      PinCircleRadius(4.f), PinQuadSideLength(7.f), PinTriangleSideLength(9.5),
      PinLineThickness(1.f), PinHoverRadius(10.f), PinOffset(0.f), MiniMapPadding(8.0f, 8.0f),
      MiniMapOffset(4.0f, 4.0f), Flags(ImNodesStyleFlags_NodeOutline | ImNodesStyleFlags_GridLines),
      Colors()
{
}

namespace IMNODES_NAMESPACE
{
ImNodesContext* CreateContext()
{
    ImNodesContext* ctx = IM_NEW(ImNodesContext)();
    if (GImNodes == NULL)
        SetCurrentContext(ctx);
    Initialize(ctx);
    return ctx;
}

void DestroyContext(ImNodesContext* ctx)
{
    if (ctx == NULL)
        ctx = GImNodes;
    Shutdown(ctx);
    if (GImNodes == ctx)
        SetCurrentContext(NULL);
    IM_DELETE(ctx);
}

ImNodesContext* GetCurrentContext() { return GImNodes; }

void SetCurrentContext(ImNodesContext* ctx) { GImNodes = ctx; }

ImNodesEditorContext* EditorContextCreate()
{
    void* mem = ImGui::MemAlloc(sizeof(ImNodesEditorContext));
    new (mem) ImNodesEditorContext();
    return (ImNodesEditorContext*)mem;
}

void EditorContextFree(ImNodesEditorContext* ctx)
{
    ctx->~ImNodesEditorContext();
    ImGui::MemFree(ctx);
}

void EditorContextSet(ImNodesEditorContext* ctx) { GImNodes->EditorCtx = ctx; }

ImVec2 EditorContextGetPanning()
{
    const ImNodesEditorContext& editor = EditorContextGet();
    return editor.Panning;
}

void EditorContextResetPanning(const ImVec2& pos)
{
    ImNodesEditorContext& editor = EditorContextGet();
    editor.Panning = pos;
}

void EditorContextMoveToNode(const int node_id)
{
    const ImNodeDrawData& node = FindNode(GImNodes->Nodes, node_id);
    ImNodesEditorContext& editor = EditorContextGet();
    editor.Panning += CalculatePanningOffsetToNode(
        GImNodes->CanvasRectScreenSpace.GetCenter(), node.BaseRectangle.GetCenter());
}

void SetImGuiContext(ImGuiContext* ctx) { ImGui::SetCurrentContext(ctx); }

ImNodesIO& GetIO() { return GImNodes->Io; }

ImNodesStyle& GetStyle() { return GImNodes->Style; }

void StyleColorsDark()
{
    GImNodes->Style.Colors[ImNodesCol_NodeBackground] = IM_COL32(50, 50, 50, 255);
    GImNodes->Style.Colors[ImNodesCol_NodeBackgroundHovered] = IM_COL32(75, 75, 75, 255);
    GImNodes->Style.Colors[ImNodesCol_NodeBackgroundSelected] = IM_COL32(75, 75, 75, 255);
    GImNodes->Style.Colors[ImNodesCol_NodeOutline] = IM_COL32(100, 100, 100, 255);
    // title bar colors match ImGui's titlebg colors
    GImNodes->Style.Colors[ImNodesCol_TitleBar] = IM_COL32(41, 74, 122, 255);
    GImNodes->Style.Colors[ImNodesCol_TitleBarHovered] = IM_COL32(66, 150, 250, 255);
    GImNodes->Style.Colors[ImNodesCol_TitleBarSelected] = IM_COL32(66, 150, 250, 255);
    // link colors match ImGui's slider grab colors
    GImNodes->Style.Colors[ImNodesCol_Link] = IM_COL32(61, 133, 224, 200);
    GImNodes->Style.Colors[ImNodesCol_LinkHovered] = IM_COL32(66, 150, 250, 255);
    GImNodes->Style.Colors[ImNodesCol_LinkSelected] = IM_COL32(66, 150, 250, 255);
    // pin colors match ImGui's button colors
    GImNodes->Style.Colors[ImNodesCol_Pin] = IM_COL32(53, 150, 250, 180);
    GImNodes->Style.Colors[ImNodesCol_PinHovered] = IM_COL32(53, 150, 250, 255);

    GImNodes->Style.Colors[ImNodesCol_BoxSelector] = IM_COL32(61, 133, 224, 30);
    GImNodes->Style.Colors[ImNodesCol_BoxSelectorOutline] = IM_COL32(61, 133, 224, 150);

    GImNodes->Style.Colors[ImNodesCol_GridBackground] = IM_COL32(40, 40, 50, 200);
    GImNodes->Style.Colors[ImNodesCol_GridLine] = IM_COL32(200, 200, 200, 40);

    // minimap colors
    GImNodes->Style.Colors[ImNodesCol_MiniMapBackground] = IM_COL32(25, 25, 25, 150);
    GImNodes->Style.Colors[ImNodesCol_MiniMapBackgroundHovered] = IM_COL32(25, 25, 25, 200);
    GImNodes->Style.Colors[ImNodesCol_MiniMapOutline] = IM_COL32(150, 150, 150, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapOutlineHovered] = IM_COL32(150, 150, 150, 200);
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackground] = IM_COL32(200, 200, 200, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundHovered] = IM_COL32(200, 200, 200, 255);
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundSelected] =
        GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundHovered];
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeOutline] = IM_COL32(200, 200, 200, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapLink] = GImNodes->Style.Colors[ImNodesCol_Link];
    GImNodes->Style.Colors[ImNodesCol_MiniMapLinkSelected] =
        GImNodes->Style.Colors[ImNodesCol_LinkSelected];
    GImNodes->Style.Colors[ImNodesCol_MiniMapCanvas] = IM_COL32(200, 200, 200, 25);
    GImNodes->Style.Colors[ImNodesCol_MiniMapCanvasOutline] = IM_COL32(200, 200, 200, 200);
}

void StyleColorsClassic()
{
    GImNodes->Style.Colors[ImNodesCol_NodeBackground] = IM_COL32(50, 50, 50, 255);
    GImNodes->Style.Colors[ImNodesCol_NodeBackgroundHovered] = IM_COL32(75, 75, 75, 255);
    GImNodes->Style.Colors[ImNodesCol_NodeBackgroundSelected] = IM_COL32(75, 75, 75, 255);
    GImNodes->Style.Colors[ImNodesCol_NodeOutline] = IM_COL32(100, 100, 100, 255);
    GImNodes->Style.Colors[ImNodesCol_TitleBar] = IM_COL32(69, 69, 138, 255);
    GImNodes->Style.Colors[ImNodesCol_TitleBarHovered] = IM_COL32(82, 82, 161, 255);
    GImNodes->Style.Colors[ImNodesCol_TitleBarSelected] = IM_COL32(82, 82, 161, 255);
    GImNodes->Style.Colors[ImNodesCol_Link] = IM_COL32(255, 255, 255, 100);
    GImNodes->Style.Colors[ImNodesCol_LinkHovered] = IM_COL32(105, 99, 204, 153);
    GImNodes->Style.Colors[ImNodesCol_LinkSelected] = IM_COL32(105, 99, 204, 153);
    GImNodes->Style.Colors[ImNodesCol_Pin] = IM_COL32(89, 102, 156, 170);
    GImNodes->Style.Colors[ImNodesCol_PinHovered] = IM_COL32(102, 122, 179, 200);
    GImNodes->Style.Colors[ImNodesCol_BoxSelector] = IM_COL32(82, 82, 161, 100);
    GImNodes->Style.Colors[ImNodesCol_BoxSelectorOutline] = IM_COL32(82, 82, 161, 255);
    GImNodes->Style.Colors[ImNodesCol_GridBackground] = IM_COL32(40, 40, 50, 200);
    GImNodes->Style.Colors[ImNodesCol_GridLine] = IM_COL32(200, 200, 200, 40);

    // minimap colors
    GImNodes->Style.Colors[ImNodesCol_MiniMapBackground] = IM_COL32(25, 25, 25, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapBackgroundHovered] = IM_COL32(25, 25, 25, 200);
    GImNodes->Style.Colors[ImNodesCol_MiniMapOutline] = IM_COL32(150, 150, 150, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapOutlineHovered] = IM_COL32(150, 150, 150, 200);
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackground] = IM_COL32(200, 200, 200, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundSelected] =
        GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundHovered];
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundSelected] = IM_COL32(200, 200, 240, 255);
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeOutline] = IM_COL32(200, 200, 200, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapLink] = GImNodes->Style.Colors[ImNodesCol_Link];
    GImNodes->Style.Colors[ImNodesCol_MiniMapLinkSelected] =
        GImNodes->Style.Colors[ImNodesCol_LinkSelected];
    GImNodes->Style.Colors[ImNodesCol_MiniMapCanvas] = IM_COL32(200, 200, 200, 25);
    GImNodes->Style.Colors[ImNodesCol_MiniMapCanvasOutline] = IM_COL32(200, 200, 200, 200);
}

void StyleColorsLight()
{
    GImNodes->Style.Colors[ImNodesCol_NodeBackground] = IM_COL32(240, 240, 240, 255);
    GImNodes->Style.Colors[ImNodesCol_NodeBackgroundHovered] = IM_COL32(240, 240, 240, 255);
    GImNodes->Style.Colors[ImNodesCol_NodeBackgroundSelected] = IM_COL32(240, 240, 240, 255);
    GImNodes->Style.Colors[ImNodesCol_NodeOutline] = IM_COL32(100, 100, 100, 255);
    GImNodes->Style.Colors[ImNodesCol_TitleBar] = IM_COL32(248, 248, 248, 255);
    GImNodes->Style.Colors[ImNodesCol_TitleBarHovered] = IM_COL32(209, 209, 209, 255);
    GImNodes->Style.Colors[ImNodesCol_TitleBarSelected] = IM_COL32(209, 209, 209, 255);
    // original imgui values: 66, 150, 250
    GImNodes->Style.Colors[ImNodesCol_Link] = IM_COL32(66, 150, 250, 100);
    // original imgui values: 117, 138, 204
    GImNodes->Style.Colors[ImNodesCol_LinkHovered] = IM_COL32(66, 150, 250, 242);
    GImNodes->Style.Colors[ImNodesCol_LinkSelected] = IM_COL32(66, 150, 250, 242);
    // original imgui values: 66, 150, 250
    GImNodes->Style.Colors[ImNodesCol_Pin] = IM_COL32(66, 150, 250, 160);
    GImNodes->Style.Colors[ImNodesCol_PinHovered] = IM_COL32(66, 150, 250, 255);
    GImNodes->Style.Colors[ImNodesCol_BoxSelector] = IM_COL32(90, 170, 250, 30);
    GImNodes->Style.Colors[ImNodesCol_BoxSelectorOutline] = IM_COL32(90, 170, 250, 150);
    GImNodes->Style.Colors[ImNodesCol_GridBackground] = IM_COL32(225, 225, 225, 255);
    GImNodes->Style.Colors[ImNodesCol_GridLine] = IM_COL32(180, 180, 180, 100);

    // minimap colors
    GImNodes->Style.Colors[ImNodesCol_MiniMapBackground] = IM_COL32(25, 25, 25, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapBackgroundHovered] = IM_COL32(25, 25, 25, 200);
    GImNodes->Style.Colors[ImNodesCol_MiniMapOutline] = IM_COL32(150, 150, 150, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapOutlineHovered] = IM_COL32(150, 150, 150, 200);
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackground] = IM_COL32(200, 200, 200, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundSelected] =
        GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundHovered];
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeBackgroundSelected] = IM_COL32(200, 200, 240, 255);
    GImNodes->Style.Colors[ImNodesCol_MiniMapNodeOutline] = IM_COL32(200, 200, 200, 100);
    GImNodes->Style.Colors[ImNodesCol_MiniMapLink] = GImNodes->Style.Colors[ImNodesCol_Link];
    GImNodes->Style.Colors[ImNodesCol_MiniMapLinkSelected] =
        GImNodes->Style.Colors[ImNodesCol_LinkSelected];
    GImNodes->Style.Colors[ImNodesCol_MiniMapCanvas] = IM_COL32(200, 200, 200, 25);
    GImNodes->Style.Colors[ImNodesCol_MiniMapCanvasOutline] = IM_COL32(200, 200, 200, 200);
}

void BeginNodeEditor()
{
    assert(GImNodes->CurrentScope == ImNodesScope_None);
    GImNodes->CurrentScope = ImNodesScope_Editor;

    // Reset state from previous pass

    ImNodesEditorContext& editor = EditorContextGet();

    editor.AutoPanningDelta = ImVec2(0, 0);
    editor.GridContentBounds = ImRect(FLT_MAX, FLT_MAX, FLT_MIN, FLT_MIN);
    editor.MiniMapEnabled = false;

    GImNodes->Nodes.resize(0);
    for (int i = 0; i < GImNodes->NodeIdxToPinIndices.size(); ++i)
    {
        ImVector<int>& pin_indices = GImNodes->NodeIdxToPinIndices[i];
        pin_indices.resize(0);
        pin_indices.~ImVector();
    }
    GImNodes->NodeIdxToPinIndices.resize(0);

    GImNodes->Pins.resize(0);
    GImNodes->PinAttributeRectangles.resize(0);
    GImNodes->PinIdToPinIdx.clear();

    GImNodes->Links.reset();

    GImNodes->NodeOverlappingCursor.Reset();

    GImNodes->HoveredNodeIdx.Reset();
    GImNodes->HoveredLinkIdx.Reset();
    GImNodes->HoveredPinIdx.Reset();
    GImNodes->DeletedLinkIdx.Reset();
    GImNodes->SnapLinkIdx.Reset();

    GImNodes->UIEvent.Reset();

    GImNodes->MousePos = ImGui::GetIO().MousePos;
    GImNodes->LeftMouseClicked = ImGui::IsMouseClicked(0);
    GImNodes->LeftMouseReleased = ImGui::IsMouseReleased(0);
    GImNodes->AltMouseClicked =
        (GImNodes->Io.EmulateThreeButtonMouse.Modifier != NULL &&
         *GImNodes->Io.EmulateThreeButtonMouse.Modifier && GImNodes->LeftMouseClicked) ||
        ImGui::IsMouseClicked(GImNodes->Io.AltMouseButton);
    GImNodes->LeftMouseDragging = ImGui::IsMouseDragging(0, 0.0f);
    GImNodes->AltMouseDragging =
        (GImNodes->Io.EmulateThreeButtonMouse.Modifier != NULL && GImNodes->LeftMouseDragging &&
         (*GImNodes->Io.EmulateThreeButtonMouse.Modifier)) ||
        ImGui::IsMouseDragging(GImNodes->Io.AltMouseButton, 0.0f);
    GImNodes->AltMouseScrollDelta = ImGui::GetIO().MouseWheel;

    GImNodes->ActiveAttribute = false;

    ImGui::BeginGroup();
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, GImNodes->Style.Colors[ImNodesCol_GridBackground]);
        ImGui::BeginChild(
            "scrolling_region",
            ImVec2(0.f, 0.f),
            true,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollWithMouse);
        GImNodes->CanvasOriginScreenSpace = ImGui::GetCursorScreenPos();

        // NOTE: we have to fetch the canvas draw list *after* we call
        // BeginChild(), otherwise the ImGui UI elements are going to be
        // rendered into the parent window draw list.
        DrawListSet(ImGui::GetWindowDrawList());

        {
            const ImVec2 canvas_size = ImGui::GetWindowSize();
            GImNodes->CanvasRectScreenSpace = ImRect(
                CanvasSpaceToScreenSpace(ImVec2(0.f, 0.f)), CanvasSpaceToScreenSpace(canvas_size));

            if (GImNodes->Style.Flags & ImNodesStyleFlags_GridLines)
            {
                DrawGrid(editor, canvas_size);
            }
        }
    }
}

void EndNodeEditor()
{
    assert(GImNodes->CurrentScope == ImNodesScope_Editor);
    GImNodes->CurrentScope = ImNodesScope_None;

    CalcLinkGeometries(GImNodes->Pins, GImNodes->Links);

    ImNodesEditorContext& editor = EditorContextGet();

    bool no_grid_content = editor.GridContentBounds.IsInverted();
    if (no_grid_content)
    {
        editor.GridContentBounds = ScreenSpaceToGridSpace(editor, GImNodes->CanvasRectScreenSpace);
    }

    // Detect ImGui interaction first, because it blocks interaction with the rest of the UI

    if (GImNodes->LeftMouseClicked && ImGui::IsAnyItemActive())
    {
        editor.ClickInteraction.Type = ImNodesClickInteractionType_ImGuiItem;
    }

    // Detect which UI element is being hovered over. Detection is done in a hierarchical fashion,
    // because a UI element being hovered excludes any other as being hovered over.

    // Don't do hovering detection for nodes/links/pins when interacting with the mini-map, since
    // its an *overlay* with its own interaction behavior and must have precedence during mouse
    // interaction.

    if ((editor.ClickInteraction.Type == ImNodesClickInteractionType_None ||
         (editor.ClickInteraction.Type == ImNodesClickInteractionType_UnconnectedLink ||
          editor.ClickInteraction.Type == ImNodesClickInteractionType_SnappedLink)) &&
        MouseInCanvas() && !IsMiniMapHovered())
    {
        GImNodes->HoveredPinIdx = ResolveHoveredPin(GImNodes->Pins);

        if (!GImNodes->HoveredPinIdx.HasValue())
        {
            // Resolve which node is actually on top and being hovered using the depth stack.
            GImNodes->HoveredNodeIdx = ResolveHoveredNode();
        }

        // We don't check for hovered pins here, because if we want to detach a link by clicking and
        // dragging, we need to have both a link and pin hovered.
        if (!GImNodes->HoveredNodeIdx.HasValue())
        {
            GImNodes->HoveredLinkIdx = ResolveHoveredLink(GImNodes->Pins, GImNodes->Links);
        }
    }

    DrawNodesAndPins(editor, GImNodes->HoveredNodeIdx, GImNodes->HoveredPinIdx);

    // In order to render the links underneath the nodes, we want to first select the bottom draw
    // channel.
    GImNodes->CanvasDrawList->ChannelsSetCurrent(0);

    DrawLinks(editor, GImNodes->Links);

    // Render the click interaction UI elements (partial links, box selector) on top of everything
    // else.

    DrawListAppendClickInteractionChannel();
    DrawListActivateClickInteractionChannel();

    if (IsMiniMapActive())
    {
        CalcMiniMapLayout();
        MiniMapUpdate();
    }

    // Handle node graph interaction

    if (not IsMiniMapHovered())
    {
        if (GImNodes->LeftMouseClicked && GImNodes->HoveredLinkIdx.HasValue())
        {
            BeginLinkInteraction(editor, GImNodes->HoveredLinkIdx.Value());
        }

        else if (GImNodes->LeftMouseClicked && GImNodes->HoveredPinIdx.HasValue())
        {
            BeginLinkCreation(editor, GImNodes->Pins[GImNodes->HoveredPinIdx.Value()].Id);
        }

        else if (GImNodes->LeftMouseClicked && GImNodes->HoveredNodeIdx.HasValue())
        {
            BeginNodeInteraction(editor, GImNodes->Nodes[GImNodes->HoveredNodeIdx.Value()].Id);
        }

        else if (
            GImNodes->LeftMouseClicked || GImNodes->LeftMouseReleased ||
            GImNodes->AltMouseClicked || GImNodes->AltMouseScrollDelta != 0.f)
        {
            BeginCanvasInteraction(editor);
        }

        bool should_auto_pan =
            editor.ClickInteraction.Type == ImNodesClickInteractionType_BoxSelection ||
            editor.ClickInteraction.Type == ImNodesClickInteractionType_UnconnectedLink ||
            editor.ClickInteraction.Type == ImNodesClickInteractionType_Node;
        if (should_auto_pan && !MouseInCanvas())
        {
            ImVec2 mouse = ImGui::GetMousePos();
            ImVec2 center = GImNodes->CanvasRectScreenSpace.GetCenter();
            ImVec2 direction = (center - mouse);
            direction = direction * ImInvLength(direction, 0.0);

            editor.AutoPanningDelta =
                direction * ImGui::GetIO().DeltaTime * GImNodes->Io.AutoPanningSpeed;
            editor.Panning += editor.AutoPanningDelta;
        }
    }
    ClickInteractionUpdate(editor);

    // Finally, merge the draw channels
    GImNodes->CanvasDrawList->ChannelsMerge();

    // pop style
    ImGui::EndChild();      // end scrolling region
    ImGui::PopStyleColor(); // pop child window background color
    ImGui::PopStyleVar();   // pop window padding
    ImGui::PopStyleVar();   // pop frame padding
    ImGui::EndGroup();
}

void MiniMap(
    const float                              minimap_size_fraction,
    const ImNodesMiniMapLocation             location,
    const ImNodesMiniMapNodeHoveringCallback node_hovering_callback,
    void*                                    node_hovering_callback_data)
{
    // Check that editor size fraction is sane; must be in the range (0, 1]
    assert(minimap_size_fraction > 0.f && minimap_size_fraction <= 1.f);

    // Remember to call before EndNodeEditor
    assert(GImNodes->CurrentScope == ImNodesScope_Editor);

    ImNodesEditorContext& editor = EditorContextGet();

    editor.MiniMapEnabled = true;
    editor.MiniMapSizeFraction = minimap_size_fraction;
    editor.MiniMapLocation = location;

    // Set node hovering callback information
    editor.MiniMapNodeHoveringCallback = node_hovering_callback;
    editor.MiniMapNodeHoveringCallbackUserData = node_hovering_callback_data;

    // Actual drawing/updating of the MiniMap is done in EndNodeEditor so that
    // mini map is draw over everything and all pin/link positions are updated
    // correctly relative to their respective nodes. Hence, we must store some of
    // of the state for the mini map in GImNodes for the actual drawing/updating
}

void BeginNode(const int node_id)
{
    // Remember to call BeginNodeEditor before calling BeginNode
    assert(GImNodes->CurrentScope == ImNodesScope_Editor);
    GImNodes->CurrentScope = ImNodesScope_Node;

    ImNodesEditorContext& editor = EditorContextGet();

    GImNodes->CurrentNodeIdx = GImNodes->Nodes.size();

    ImNodeDrawData node;
    node.Id = node_id;
    node.ColorStyle.Background = GImNodes->Style.Colors[ImNodesCol_NodeBackground];
    node.ColorStyle.BackgroundHovered = GImNodes->Style.Colors[ImNodesCol_NodeBackgroundHovered];
    node.ColorStyle.BackgroundSelected = GImNodes->Style.Colors[ImNodesCol_NodeBackgroundSelected];
    node.ColorStyle.Outline = GImNodes->Style.Colors[ImNodesCol_NodeOutline];
    node.ColorStyle.Titlebar = GImNodes->Style.Colors[ImNodesCol_TitleBar];
    node.ColorStyle.TitlebarHovered = GImNodes->Style.Colors[ImNodesCol_TitleBarHovered];
    node.ColorStyle.TitlebarSelected = GImNodes->Style.Colors[ImNodesCol_TitleBarSelected];
    node.LayoutStyle.CornerRounding = GImNodes->Style.NodeCornerRounding;
    node.LayoutStyle.Padding = GImNodes->Style.NodePadding;
    node.LayoutStyle.BorderThickness = GImNodes->Style.NodeBorderThickness;

    {
        std::map<int, ImVec2>::const_iterator id_node_pair =
            editor.GridSpaceNodeOrigins.find(node_id);
        if (id_node_pair != editor.GridSpaceNodeOrigins.end())
        {
            node.CanvasSpacePosition = GridSpaceToCanvasSpace(editor, id_node_pair->second);
        }
        else
        {
            const ImVec2 default_position = ImVec2(0.0f, 0.0f);
            const ImVec2 grid_space_position = CanvasSpaceToGridSpace(editor, default_position);
            editor.GridSpaceNodeOrigins.insert(std::make_pair(node_id, grid_space_position));
            node.CanvasSpacePosition = default_position;
        }
    }

    // Append the draw state
    {
        DrawListAppendNodeChannels();
        DrawListActivateNodeForeground(GImNodes->Nodes.size());
    }

    GImNodes->Nodes.push_back(node);

    // Start tracking which pins belong to this node.
    GImNodes->NodeIdxToPinIndices.push_back(ImVector<int>());

    // The cursor is offset so that the user's ImGui widgets satisfy the Padding setting that the
    // user has specified.
    //
    // IMPLEMENTATION NOTES:
    // - ImGui::SetCursorPos sets the cursor position, relative to the current widget -- in this
    // case the child object which was created in BeginNodeEditor().
    // - We could also use ImGui::SetCursorScreenSpacePos to set the cursor position in screen space
    // directly.
    ImGui::SetCursorPos(node.CanvasSpacePosition + node.LayoutStyle.Padding);

    ImGui::PushID(node.Id);
    ImGui::BeginGroup();
}

void EndNode()
{
    assert(GImNodes->CurrentScope == ImNodesScope_Node);
    GImNodes->CurrentScope = ImNodesScope_Editor;

    // The node's rectangle depends on the ImGui UI group size.
    ImGui::EndGroup();
    ImGui::PopID();

    ImNodeDrawData& node = GImNodes->Nodes.back();
    // Calculate the rectangle which fits tightly around the node's UI content group.
    node.BaseRectangle = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    node.BaseRectangle.Expand(node.LayoutStyle.Padding);

    ImNodesEditorContext& editor = EditorContextGet();

    const ImVec2 node_grid_space_pos = CanvasSpaceToGridSpace(editor, node.CanvasSpacePosition);
    editor.GridContentBounds.Add(node_grid_space_pos);
    editor.GridContentBounds.Add(node_grid_space_pos + node.BaseRectangle.GetSize());

    {
        const ImVector<int>&    pin_indices = GImNodes->NodeIdxToPinIndices.back();
        const ImVector<ImRect>& pin_attribute_rects = GImNodes->PinAttributeRectangles;

        for (int i = 0; i < pin_indices.size(); ++i)
        {
            const int pin_idx = pin_indices[i];

            ImPinData& pin_draw_state = GImNodes->Pins[pin_idx];
            pin_draw_state.ScreenSpacePosition = GetScreenSpacePinCoordinates(
                node.BaseRectangle, pin_attribute_rects[pin_idx], pin_draw_state.Type);
        }
    }

    if (node.BaseRectangle.Contains(GImNodes->MousePos))
    {
        GImNodes->NodeOverlappingCursor = GImNodes->CurrentNodeIdx;
    }
}

ImVec2 GetNodeDimensions(const int node_id)
{
    const ImNodeDrawData*       node = GImNodes->Nodes.begin();
    const ImNodeDrawData* const nodes_end = GImNodes->Nodes.end();

    while (node < nodes_end)
    {
        if ((node++)->Id == node_id)
        {
            break;
        }
    }

    assert(node != nodes_end);

    return node->BaseRectangle.GetSize();
}

void BeginNodeTitleBar()
{
    assert(GImNodes->CurrentScope == ImNodesScope_Node);
    ImGui::BeginGroup();
}

void EndNodeTitleBar()
{
    assert(GImNodes->CurrentScope == ImNodesScope_Node);
    ImGui::EndGroup();

    ImNodeDrawData& node = GImNodes->Nodes.back();

    // Get a rectangle which fits tightly around the node's title bar content.
    node.TitleRectangle = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    ImGui::ItemAdd(GetNodeTitleRect(node), ImGui::GetID("title_bar"));

    {
        // vertical offset = (height) + 2 * (vertical padding)
        const ImVec2 title_bar_vertical_offset =
            ImVec2(0.0f, node.TitleRectangle.GetHeight() + 2.0f * node.LayoutStyle.Padding.y);
        const ImVec2 node_content_pos =
            node.CanvasSpacePosition + node.LayoutStyle.Padding + title_bar_vertical_offset;

        ImGui::SetCursorPos(node_content_pos);
    }
}

void BeginInputAttribute(const int id, const ImNodesPinShape shape)
{
    BeginPinAttribute(id, ImNodesAttributeType_Input, shape, GImNodes->Nodes.size() - 1);
}

void EndInputAttribute() { EndPinAttribute(); }

void BeginOutputAttribute(const int id, const ImNodesPinShape shape)
{
    BeginPinAttribute(id, ImNodesAttributeType_Output, shape, GImNodes->Nodes.size() - 1);
}

void EndOutputAttribute() { EndPinAttribute(); }

void BeginStaticAttribute(const int id)
{
    // Make sure to call BeginNode() before calling BeginAttribute()
    assert(GImNodes->CurrentScope == ImNodesScope_Node);
    GImNodes->CurrentScope = ImNodesScope_Attribute;

    GImNodes->CurrentAttributeId = id;

    ImGui::BeginGroup();
    ImGui::PushID(id);
}

void EndStaticAttribute()
{
    // Make sure to call BeginNode() before calling BeginAttribute()
    assert(GImNodes->CurrentScope == ImNodesScope_Attribute);
    GImNodes->CurrentScope = ImNodesScope_Node;

    ImGui::PopID();
    ImGui::EndGroup();

    if (ImGui::IsItemActive())
    {
        GImNodes->ActiveAttribute = true;
        GImNodes->ActiveAttributeId = GImNodes->CurrentAttributeId;
    }
}

void PushAttributeFlag(const ImNodesAttributeFlags flag)
{
    GImNodes->CurrentAttributeFlags |= flag;
    GImNodes->AttributeFlagStack.push_back(GImNodes->CurrentAttributeFlags);
}

void PopAttributeFlag()
{
    // PopAttributeFlag called without a matching PushAttributeFlag!
    // The bottom value is always the default value, pushed in Initialize().
    assert(GImNodes->AttributeFlagStack.size() > 1);

    GImNodes->AttributeFlagStack.pop_back();
    GImNodes->CurrentAttributeFlags = GImNodes->AttributeFlagStack.back();
}

void Link(const int id, const int start_attr_id, const int end_attr_id)
{
    assert(GImNodes->CurrentScope == ImNodesScope_Editor);

    const int link_idx = GImNodes->Links.size();
    GImNodes->Links.Data.push_back(
        ImLinkData(id, start_attr_id, end_attr_id, GImNodes->Style.Colors));

    const ImNodesEditorContext& editor = EditorContextGet();

    if (editor.ClickInteraction.Type == ImNodesClickInteractionType_SnappedLink &&
        ((editor.ClickInteraction.SnappedLink.StartPinId == start_attr_id &&
          editor.ClickInteraction.SnappedLink.SnappedPinId == end_attr_id) ||
         (editor.ClickInteraction.SnappedLink.StartPinId == end_attr_id &&
          editor.ClickInteraction.SnappedLink.SnappedPinId == start_attr_id)))
    {
        GImNodes->SnapLinkIdx = link_idx;
    }
}

void PushColorStyle(const ImNodesCol item, unsigned int color)
{
    GImNodes->ColorModifierStack.push_back(ImNodesColElement(GImNodes->Style.Colors[item], item));
    GImNodes->Style.Colors[item] = color;
}

void PopColorStyle()
{
    assert(GImNodes->ColorModifierStack.size() > 0);
    const ImNodesColElement elem = GImNodes->ColorModifierStack.back();
    GImNodes->Style.Colors[elem.Item] = elem.Color;
    GImNodes->ColorModifierStack.pop_back();
}

struct ImNodesStyleVarInfo
{
    ImGuiDataType Type;
    ImU32         Count;
    ImU32         Offset;
    void* GetVarPtr(ImNodesStyle* style) const { return (void*)((unsigned char*)style + Offset); }
};

static const ImNodesStyleVarInfo GStyleVarInfo[] = {
    // ImNodesStyleVar_GridSpacing
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, GridSpacing)},
    // ImNodesStyleVar_NodeCornerRounding
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, NodeCornerRounding)},
    // ImNodesStyleVar_NodePadding
    {ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImNodesStyle, NodePadding)},
    // ImNodesStyleVar_NodeBorderThickness
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, NodeBorderThickness)},
    // ImNodesStyleVar_LinkThickness
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, LinkThickness)},
    // ImNodesStyleVar_LinkLineSegmentsPerLength
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, LinkLineSegmentsPerLength)},
    // ImNodesStyleVar_LinkHoverDistance
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, LinkHoverDistance)},
    // ImNodesStyleVar_PinCircleRadius
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, PinCircleRadius)},
    // ImNodesStyleVar_PinQuadSideLength
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, PinQuadSideLength)},
    // ImNodesStyleVar_PinTriangleSideLength
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, PinTriangleSideLength)},
    // ImNodesStyleVar_PinLineThickness
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, PinLineThickness)},
    // ImNodesStyleVar_PinHoverRadius
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, PinHoverRadius)},
    // ImNodesStyleVar_PinOffset
    {ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImNodesStyle, PinOffset)},
    // ImNodesStyleVar_MiniMapPadding
    {ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImNodesStyle, MiniMapPadding)},
    // ImNodesStyleVar_MiniMapOffset
    {ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImNodesStyle, MiniMapOffset)},
};

static const ImNodesStyleVarInfo* GetStyleVarInfo(ImNodesStyleVar idx)
{
    IM_ASSERT(idx >= 0 && idx < ImNodesStyleVar_COUNT);
    IM_ASSERT(IM_ARRAYSIZE(GStyleVarInfo) == ImNodesStyleVar_COUNT);
    return &GStyleVarInfo[idx];
}

void PushStyleVar(const ImNodesStyleVar item, const float value)
{
    const ImNodesStyleVarInfo* var_info = GetStyleVarInfo(item);
    if (var_info->Type == ImGuiDataType_Float && var_info->Count == 1)
    {
        float& style_var = *(float*)var_info->GetVarPtr(&GImNodes->Style);
        GImNodes->StyleModifierStack.push_back(ImNodesStyleVarElement(item, style_var));
        style_var = value;
        return;
    }
    IM_ASSERT(0 && "Called PushStyleVar() float variant but variable is not a float!");
}

void PushStyleVar(const ImNodesStyleVar item, const ImVec2& value)
{
    const ImNodesStyleVarInfo* var_info = GetStyleVarInfo(item);
    if (var_info->Type == ImGuiDataType_Float && var_info->Count == 2)
    {
        ImVec2& style_var = *(ImVec2*)var_info->GetVarPtr(&GImNodes->Style);
        GImNodes->StyleModifierStack.push_back(ImNodesStyleVarElement(item, style_var));
        style_var = value;
        return;
    }
    IM_ASSERT(0 && "Called PushStyleVar() ImVec2 variant but variable is not a ImVec2!");
}

void PopStyleVar(int count)
{
    while (count > 0)
    {
        assert(GImNodes->StyleModifierStack.size() > 0);
        const ImNodesStyleVarElement style_backup = GImNodes->StyleModifierStack.back();
        GImNodes->StyleModifierStack.pop_back();
        const ImNodesStyleVarInfo* var_info = GetStyleVarInfo(style_backup.Item);
        void*                      style_var = var_info->GetVarPtr(&GImNodes->Style);
        if (var_info->Type == ImGuiDataType_Float && var_info->Count == 1)
        {
            ((float*)style_var)[0] = style_backup.FloatValue[0];
        }
        else if (var_info->Type == ImGuiDataType_Float && var_info->Count == 2)
        {
            ((float*)style_var)[0] = style_backup.FloatValue[0];
            ((float*)style_var)[1] = style_backup.FloatValue[1];
        }
        count--;
    }
}

void SetNodeScreenSpacePos(const int node_id, const ImVec2& screen_space_pos)
{
    ImNodesEditorContext& editor = EditorContextGet();
    editor.GridSpaceNodeOrigins[node_id] = ScreenSpaceToGridSpace(editor, screen_space_pos);
}

void SetNodeCanvasSpacePos(const int node_id, const ImVec2& canvas_space_pos)
{
    ImNodesEditorContext& editor = EditorContextGet();
    editor.GridSpaceNodeOrigins[node_id] = CanvasSpaceToGridSpace(editor, canvas_space_pos);
}

void SetNodeGridSpacePos(const int node_id, const ImVec2& grid_pos)
{
    ImNodesEditorContext& editor = EditorContextGet();
    editor.GridSpaceNodeOrigins[node_id] = grid_pos;
}

ImVec2 GetNodeScreenSpacePos(const int node_id)
{
    const ImNodeDrawData& node = FindNode(GImNodes->Nodes, node_id);
    return CanvasSpaceToScreenSpace(node.CanvasSpacePosition);
}

ImVec2 GetNodeCanvasSpacePos(const int node_id)
{
    const ImNodeDrawData& node = FindNode(GImNodes->Nodes, node_id);
    return node.CanvasSpacePosition;
}

ImVec2 GetNodeGridSpacePos(const int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    const ImNodeDrawData& node = FindNode(GImNodes->Nodes, node_id);
    return CanvasSpaceToGridSpace(editor, node.CanvasSpacePosition);
}

bool IsEditorHovered() { return MouseInCanvas(); }

bool IsNodeHovered(int* const node_id)
{
    assert(GImNodes->CurrentScope == ImNodesScope_None);
    assert(node_id != NULL);

    const bool is_hovered = GImNodes->HoveredNodeIdx.HasValue();
    if (is_hovered)
    {
        *node_id = GImNodes->Nodes[GImNodes->HoveredNodeIdx.Value()].Id;
    }
    return is_hovered;
}

bool IsLinkHovered(int* const link_id)
{
    assert(GImNodes->CurrentScope == ImNodesScope_None);
    assert(link_id != NULL);

    const bool is_hovered = GImNodes->HoveredLinkIdx.HasValue();
    if (is_hovered)
    {
        *link_id = GImNodes->Links.Data[GImNodes->HoveredLinkIdx.Value()].LinkId;
    }
    return is_hovered;
}

bool IsPinHovered(int* const attr)
{
    assert(GImNodes->CurrentScope == ImNodesScope_None);
    assert(attr != NULL);

    const bool is_hovered = GImNodes->HoveredPinIdx.HasValue();
    if (is_hovered)
    {
        *attr = GImNodes->Pins[GImNodes->HoveredPinIdx.Value()].Id;
    }
    return is_hovered;
}

int NumSelectedNodes()
{
    assert(GImNodes->CurrentScope == ImNodesScope_None);
    const ImNodesEditorContext& editor = EditorContextGet();
    return editor.SelectedNodeIds.size();
}

int NumSelectedLinks()
{
    assert(GImNodes->CurrentScope == ImNodesScope_None);
    const ImNodesEditorContext& editor = EditorContextGet();
    return editor.SelectedLinkIds.size();
}

void GetSelectedNodes(int* node_ids)
{
    assert(node_ids != NULL);

    const ImNodesEditorContext& editor = EditorContextGet();
    assert(!editor.SelectedNodeIds.empty());

    memcpy(node_ids, editor.SelectedNodeIds.Data, size_t(editor.SelectedNodeIds.size_in_bytes()));
}

void GetSelectedLinks(int* link_ids)
{
    assert(link_ids != NULL);

    const ImNodesEditorContext& editor = EditorContextGet();
    assert(!editor.SelectedLinkIds.empty());

    memcpy(link_ids, editor.SelectedLinkIds.Data, size_t(editor.SelectedLinkIds.size_in_bytes()));
}

void ClearNodeSelection()
{
    ImNodesEditorContext& editor = EditorContextGet();
    editor.SelectedNodeIds.resize(0);
}

void ClearNodeSelection(int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    editor.SelectedNodeIds.find_erase_unsorted(node_id);
}

void ClearLinkSelection()
{
    ImNodesEditorContext& editor = EditorContextGet();
    editor.SelectedLinkIds.resize(0);
}

void ClearLinkSelection(int link_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    editor.SelectedLinkIds.find_erase_unsorted(link_id);
}

void SelectNode(int node_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    editor.SelectedNodeIds.push_back(node_id);
}

void SelectLink(int link_id)
{
    ImNodesEditorContext& editor = EditorContextGet();
    editor.SelectedLinkIds.push_back(link_id);
}

bool IsNodeSelected(int node_id)
{
    const ImNodesEditorContext& editor = EditorContextGet();
    return editor.SelectedNodeIds.contains(node_id);
}

bool IsLinkSelected(int link_id)
{
    const ImNodesEditorContext& editor = EditorContextGet();
    return editor.SelectedLinkIds.contains(link_id);
}

bool IsAttributeActive()
{
    assert((GImNodes->CurrentScope & ImNodesScope_Node) != 0);

    if (!GImNodes->ActiveAttribute)
    {
        return false;
    }

    return GImNodes->ActiveAttributeId == GImNodes->CurrentAttributeId;
}

bool IsAnyAttributeActive(int* const attribute_id)
{
    assert((GImNodes->CurrentScope & (ImNodesScope_Node | ImNodesScope_Attribute)) == 0);

    if (!GImNodes->ActiveAttribute)
    {
        return false;
    }

    if (attribute_id != NULL)
    {
        *attribute_id = GImNodes->ActiveAttributeId;
    }

    return true;
}

bool IsLinkStarted(int* const started_at_id)
{
    // Call this function after EndNodeEditor()!
    assert(GImNodes->CurrentScope == ImNodesScope_None);
    assert(started_at_id != NULL);

    if (GImNodes->UIEvent.IsLinkStarted())
    {
        *started_at_id = GImNodes->UIEvent.LinkStarted.StartPinId;
        return true;
    }

    return false;
}

bool IsLinkDropped(int* const started_at_id, const bool including_detached_links)
{
    // Call this function after EndNodeEditor()!
    assert(GImNodes->CurrentScope == ImNodesScope_None);

    const bool link_dropped_query =
        GImNodes->UIEvent.IsLinkDropped() &&
        (including_detached_links ||
         GImNodes->UIEvent.LinkDropped.CreatedFromType != ImNodesLinkCreatedFrom_Detach);

    if (link_dropped_query)
    {
        *started_at_id = GImNodes->UIEvent.LinkDropped.StartPinId;
    }

    return link_dropped_query;
}

bool IsLinkCreated(
    int* const  started_at_pin_id,
    int* const  ended_at_pin_id,
    bool* const created_from_snap)
{
    assert(GImNodes->CurrentScope == ImNodesScope_None);
    assert(started_at_pin_id != NULL);
    assert(ended_at_pin_id != NULL);

    const bool is_created = GImNodes->UIEvent.IsLinkCreated();

    if (is_created)
    {
        const ImLinkCreatedEvent& event = GImNodes->UIEvent.LinkCreated;

        // The start pin is needed to figure out which way the link was created (i.e. from output to
        // input).
        assert(GImNodes->PinIdToPinIdx.find(event.StartPinId) != GImNodes->PinIdToPinIdx.end());
        const int        start_pin_idx = GImNodes->PinIdToPinIdx[event.StartPinId];
        const ImPinData& start_pin = GImNodes->Pins[start_pin_idx];

        if (start_pin.Type == ImNodesAttributeType_Output)
        {
            *started_at_pin_id = event.StartPinId;
            *ended_at_pin_id = event.EndPinId;
        }
        else
        {
            *started_at_pin_id = event.EndPinId;
            *ended_at_pin_id = event.StartPinId;
        }

        if (created_from_snap)
        {
            *created_from_snap = event.CreatedFromType == ImNodesLinkCreatedFrom_Detach;
        }
    }

    return is_created;
}

bool IsLinkCreated(
    int*  started_at_node_id,
    int*  started_at_pin_id,
    int*  ended_at_node_id,
    int*  ended_at_pin_id,
    bool* created_from_snap)
{
    assert(GImNodes->CurrentScope == ImNodesScope_None);
    assert(started_at_node_id != NULL);
    assert(started_at_pin_id != NULL);
    assert(ended_at_node_id != NULL);
    assert(ended_at_pin_id != NULL);

    const bool is_created = GImNodes->UIEvent.IsLinkCreated();

    if (is_created)
    {
        const ImLinkCreatedEvent& event = GImNodes->UIEvent.LinkCreated;

        assert(GImNodes->PinIdToPinIdx.find(event.StartPinId) != GImNodes->PinIdToPinIdx.end());
        assert(GImNodes->PinIdToPinIdx.find(event.EndPinId) != GImNodes->PinIdToPinIdx.end());

        const int        start_pin_idx = GImNodes->PinIdToPinIdx[event.StartPinId];
        const int        end_pin_idx = GImNodes->PinIdToPinIdx[event.EndPinId];
        const ImPinData& start_pin = GImNodes->Pins[start_pin_idx];
        const ImPinData& end_pin = GImNodes->Pins[end_pin_idx];
        const int        start_parent_node_id = GImNodes->Nodes[start_pin.ParentNodeIdx].Id;
        const int        end_parent_node_id = GImNodes->Nodes[end_pin.ParentNodeIdx].Id;

        if (start_pin.Type == ImNodesAttributeType_Output)
        {
            *started_at_pin_id = event.StartPinId;
            *started_at_node_id = start_parent_node_id;
            *ended_at_pin_id = event.EndPinId;
            *ended_at_node_id = end_parent_node_id;
        }
        else
        {
            *started_at_pin_id = event.EndPinId;
            *started_at_node_id = end_parent_node_id;
            *ended_at_pin_id = event.StartPinId;
            *ended_at_node_id = start_parent_node_id;
        }

        if (created_from_snap)
        {
            *created_from_snap = event.CreatedFromType == ImNodesLinkCreatedFrom_Detach;
        }
    }

    return is_created;
}

bool IsLinkDestroyed(int* const link_id)
{
    assert(GImNodes->CurrentScope == ImNodesScope_None);

    const bool link_destroyed = GImNodes->DeletedLinkIdx.HasValue();
    if (link_destroyed)
    {
        const int link_idx = GImNodes->DeletedLinkIdx.Value();
        *link_id = GImNodes->Links.Data[link_idx].LinkId;
    }

    return link_destroyed;
}

namespace
{
void NodeLineHandler(ImNodesEditorContext& editor, const char* const line)
{
    static std::pair<int, ImVec2> deserialized_node;
    int                           x, y;

    // Each node will have the following entry:
    //
    // [node.<id>]
    // origin=<x>,<y>
    //
    // This single funtion has to be able to parse both lines.
    sscanf(line, "[node.%i", &deserialized_node.first);

    if (sscanf(line, "origin=%i,%i", &x, &y) == 2)
    {
        // Precondition: the node should not exist
        assert(
            editor.GridSpaceNodeOrigins.find(deserialized_node.first) ==
            editor.GridSpaceNodeOrigins.end());

        deserialized_node.second = ImVec2((float)x, (float)y);
        editor.GridSpaceNodeOrigins.insert(deserialized_node);
    }
}

void EditorLineHandler(ImNodesEditorContext& editor, const char* const line)
{
    (void)sscanf(line, "panning=%f,%f", &editor.Panning.x, &editor.Panning.y);
}
} // namespace

const char* SaveCurrentEditorStateToIniString(size_t* const data_size)
{
    return SaveEditorStateToIniString(&EditorContextGet(), data_size);
}

const char* SaveEditorStateToIniString(
    const ImNodesEditorContext* const editor_ptr,
    size_t* const                     data_size)
{
    assert(editor_ptr != NULL);
    const ImNodesEditorContext& editor = *editor_ptr;

    GImNodes->TextBuffer.clear();
    // TODO: check to make sure that the estimate is the upper bound of element
    GImNodes->TextBuffer.reserve(64 * editor.GridSpaceNodeOrigins.size());

    GImNodes->TextBuffer.appendf(
        "[editor]\npanning=%i,%i\n", (int)editor.Panning.x, (int)editor.Panning.y);

    for (std::map<int, ImVec2>::const_iterator iter = editor.GridSpaceNodeOrigins.begin();
         iter != editor.GridSpaceNodeOrigins.end();
         ++iter)
    {
        const int     node_id = iter->first;
        const ImVec2& grid_space_origin = iter->second;
        GImNodes->TextBuffer.appendf("\n[node.%d]\n", node_id);
        GImNodes->TextBuffer.appendf(
            "origin=%i,%i\n", (int)grid_space_origin.x, (int)grid_space_origin.y);
    }

    if (data_size != NULL)
    {
        *data_size = GImNodes->TextBuffer.size();
    }

    return GImNodes->TextBuffer.c_str();
}

void LoadCurrentEditorStateFromIniString(const char* const data, const size_t data_size)
{
    LoadEditorStateFromIniString(&EditorContextGet(), data, data_size);
}

void LoadEditorStateFromIniString(
    ImNodesEditorContext* const editor_ptr,
    const char* const           data,
    const size_t                data_size)
{
    if (data_size == 0u)
    {
        return;
    }

    ImNodesEditorContext& editor = editor_ptr == NULL ? EditorContextGet() : *editor_ptr;

    char*       buf = (char*)ImGui::MemAlloc(data_size + 1);
    const char* buf_end = buf + data_size;
    memcpy(buf, data, data_size);
    buf[data_size] = 0;

    void (*line_handler)(ImNodesEditorContext&, const char*);
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

void SaveEditorStateToIniFile(const ImNodesEditorContext* const editor, const char* const file_name)
{
    size_t      data_size = 0u;
    const char* data = SaveEditorStateToIniString(editor, &data_size);
    FILE*       file = ImFileOpen(file_name, "wt");
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

void LoadEditorStateFromIniFile(ImNodesEditorContext* const editor, const char* const file_name)
{
    size_t data_size = 0u;
    char*  file_data = (char*)ImFileLoadToMemory(file_name, "rb", &data_size);

    if (!file_data)
    {
        return;
    }

    LoadEditorStateFromIniString(editor, file_data, data_size);
    ImGui::MemFree(file_data);
}
} // namespace IMNODES_NAMESPACE
