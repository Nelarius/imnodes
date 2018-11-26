#include "imnodes.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "SDL_keycode.h"

#include <algorithm> // for std::sort
#include <assert.h>
#include <functional> // for std::greater<>
#include <string.h>   // strlen, strncmp
#include <stdio.h>    // for ssprintf, sscanf
#include <stdlib.h>

namespace imnodes
{
namespace
{
// Additional ImGui math operators
inline ImVec2 operator*(const float lhs, const ImVec2& rhs)
{
    return ImVec2(lhs * rhs.x, lhs * rhs.y);
}

inline ImVec2 operator*(const ImVec2& lhs, const float rhs)
{
    return ImVec2(lhs.x * rhs, lhs.y * rhs);
}

// Grid appearance
static const float GRID_SIZE = 32.f;

// Node appearance
static const float NODE_CORNER_ROUNDNESS = 4.0f;
static const ImVec2 NODE_CONTENT_PADDING = ImVec2(8.f, 8.f);
static const ImVec2 NODE_DUMMY_SPACING = ImVec2(80.f, 20.f);

static const float LINK_THICKNESS = 3.f;
static const float LINK_SEGMENTS_PER_LENGTH = 0.1f;
static const float LINK_HOVER_DISTANCE = 7.0f;

static const float NODE_PIN_RADIUS = 4.f;
static const float NODE_PIN_HOVER_RADIUS = 10.f;

static const size_t NODE_NAME_STR_LEN = 32u;

static const int INVALID_INDEX = -1;

enum ScopeFlags
{
    SCOPE_NONE = 0,
    SCOPE_EDITOR = 1 << 0,
    SCOPE_NODE = 1 << 2,
    SCOPE_ATTRIBUTE = 1 << 3
};

enum ImGuiChannels
{
    CHANNEL_BACKGROUND = 0,
    CHANNEL_FOREGROUND,
    CHANNEL_UI,
    CHANNEL_COUNT
};

struct Node
{
    int id;
    char name[NODE_NAME_STR_LEN];
    ImVec2 origin;
    ImRect content_rect;
    ImU32 color_styles[ColorStyle_Count];

    ImVector<ImRect> input_attributes;
    ImVector<ImRect> output_attributes;

    Node()
        : id(0u),
          name(
              "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"),
          origin(100.0f, 100.0f),
          content_rect(ImVec2(0.0f, 0.0f), ImVec2(0.0f, 0.0f)), color_styles(),
          input_attributes(), output_attributes()
    {
    }
};

struct Pin
{
    int node_idx;
    int attribute_idx;
    AttributeType type;

    Pin()
        : node_idx(INVALID_INDEX), attribute_idx(INVALID_INDEX),
          type(AttributeType_None)
    {
    }
    Pin(int nidx, int aindx, AttributeType t)
        : node_idx(nidx), attribute_idx(aindx), type(t)
    {
    }
};

bool operator==(const Pin& lhs, const Pin& rhs)
{
    return lhs.node_idx == rhs.node_idx &&
           lhs.attribute_idx == rhs.attribute_idx && lhs.type == rhs.type;
}

struct Link
{
    Pin pin1;
    Pin pin2;

    Link() : pin1(), pin2() {}
};

struct LinkRenderable
{
    // the bezier curve control points
    ImVec2 p0, p1, p2, p3;
    int num_segments;
};

// This is used to initialize the boolean flag in the static struct
struct InitializeGuard
{
    bool initialized;

    InitializeGuard() : initialized(false) {}
};

static struct
{
    InitializeGuard guard;
    EditorContext* default_editor_ctx;
    EditorContext* editor_ctx;
    ImVec2 grid_origin;
    ScopeFlags current_scope;

    ImU32 color_styles[ColorStyle_Count];
    ImVector<ImU32> color_style_stack;

    Link link_dragged;

    struct
    {
        int index;
        struct
        {
            AttributeType type;
            int index;
        } attribute;
    } current_node;

    struct
    {
        int index;
        int attribute;
    } active_node;

    struct
    {
        int index;
        ImVec2 position;
    } moved_node;

    int hovered_node;
    int selected_node;
    int hovered_link;
    int selected_link;

    ImGuiTextBuffer text_buffer;
} g;

EditorContext& editor_context_get()
{
    assert(g.editor_ctx != nullptr);
    return *g.editor_ctx;
}

inline ImVec2 editor_space_to_screen_space(const ImVec2& v)
{
    return g.grid_origin + v;
}

inline ImRect get_item_rect()
{
    return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

inline ImVec2 get_node_title_origin(const Node& node)
{
    return node.origin + NODE_CONTENT_PADDING;
}

inline ImVec2 get_node_content_origin(const Node& node)
{
    ImVec2 title_rect_height = ImVec2(
        0.f, ImGui::CalcTextSize(node.name).y + 2.f * NODE_CONTENT_PADDING.y);
    return node.origin + NODE_CONTENT_PADDING + title_rect_height;
}

inline ImRect get_title_bar_rect(const Node& node)
{
    ImVec2 ss_node_origin = editor_space_to_screen_space(node.origin);
    // TODO: lots of repetition of ImGui::CalcTextSize()
    // it should be calculated once and stored (in the node?)
    ImVec2 text_size = ImGui::CalcTextSize(node.name);
    ImVec2 min = ss_node_origin;
    ImVec2 max = ImVec2(
        node.content_rect.Max.x + NODE_CONTENT_PADDING.x,
        ss_node_origin.y + text_size.y + 2.f * NODE_CONTENT_PADDING.y);
    // NOTE: the content rect already contains 1 x NODE_CONTENT_PADDING due to
    // setting the cursor!
    return ImRect(min, max);
}

inline ImRect get_node_rect(const Node& node)
{
    float text_height =
        ImGui::CalcTextSize(node.name).y + 2.f * NODE_CONTENT_PADDING.y;

    ImRect rect = node.content_rect;
    rect.Expand(NODE_CONTENT_PADDING);
    rect.Min.y = rect.Min.y - text_height;
    return rect;
}

inline bool is_mouse_hovering_near_point(const ImVec2& point, float radius)
{
    ImVec2 delta = ImGui::GetIO().MousePos - point;
    return (delta.x * delta.x + delta.y * delta.y) < (radius * radius);
}

inline ImVec2 eval_bezier(
    float t,
    const ImVec2 p0,
    const ImVec2 p1,
    const ImVec2 p2,
    const ImVec2 p3)
{
    // B(t) = (1-t)**3 p0 + 3(1 - t)**2 t P1 + 3(1-t)t**2 P2 + t**3 P3
    return ImVec2(
        (1 - t) * (1 - t) * (1 - t) * p0.x + 3 * (1 - t) * (1 - t) * t * p1.x +
            3 * (1 - t) * t * t * p2.x + t * t * t * p3.x,
        (1 - t) * (1 - t) * (1 - t) * p0.y + 3 * (1 - t) * (1 - t) * t * p1.y +
            3 * (1 - t) * t * t * p2.y + t * t * t * p3.y);
}

// Divides the bezier curve into n segments. Evaluates the distance to each
// segment. Chooses the segment with the smallest distance, and repeats the
// algorithm on that segment, for the given number of iterations.
inline float get_closest_point_on_cubic_bezier(
    const int num_iterations,
    const int num_segments,
    const ImVec2 pos,
    const ImVec2 p0,
    const ImVec2 p1,
    const ImVec2 p2,
    const ImVec2 p3)
{
    assert(num_iterations > 0 && num_segments > 0);
    float tstart = 0.0f;
    float tend = 1.0f;
    float tbest = 0.5f;
    float best_distance = FLT_MAX;

    for (int i = 0; i < num_iterations; i++)
    {
        // split the current t-range to segments
        const float dt = (tend - tstart) / num_segments;
        for (int s = 0; s < num_segments; s++)
        {
            const float tmid = tstart + dt * (float(s) + 0.5f);
            ImVec2 bt = eval_bezier(tmid, p0, p1, p2, p3);
            ImVec2 dv = bt - pos;
            float cur_distance = ImLengthSqr(dv);
            if (cur_distance < best_distance)
            {
                best_distance = cur_distance;
                tbest = tmid;
            }
        }
        // shrink the current t-range to the best segment
        tstart = tbest - 0.5f * dt;
        tend = tbest + 0.5f * dt;
    }

    return tbest;
}

inline float get_distance_to_cubic_bezier(
    const ImVec2& pos,
    const ImVec2& p0,
    const ImVec2& p1,
    const ImVec2& p2,
    const ImVec2& p3)
{
    const int segments = 5;
    const float length = ImSqrt(ImLengthSqr(p3 - p2)) +
                         ImSqrt(ImLengthSqr(p2 - p1)) +
                         ImSqrt(ImLengthSqr(p1 - p0));
    const float iterations_per_length = 0.01f;
    const int iterations =
        int(ImClamp(length * iterations_per_length, 2.0f, 8.f));

    float t = get_closest_point_on_cubic_bezier(
        iterations, segments, pos, p0, p1, p2, p3);
    ImVec2 point_on_curve = eval_bezier(t, p0, p1, p2, p3);

    const ImVec2 to_curve = point_on_curve - pos;
    return ImSqrt(ImLengthSqr(to_curve));
}

inline LinkRenderable get_link_renderable(
    const ImVec2& output_pos,
    const ImVec2& input_pos)
{
    const ImVec2 delta = input_pos - output_pos;
    const float link_length = ImSqrt(ImLengthSqr(delta));
    // const float sign = delta.x < 0.0f ? -1.f : 1.f;
    LinkRenderable renderable;
    const ImVec2 offset = ImVec2(0.25f * link_length, 0.f);
    renderable.p0 = output_pos;
    renderable.p1 = output_pos + offset;
    renderable.p2 = input_pos - offset;
    renderable.p3 = input_pos;
    renderable.num_segments =
        ImMax(int(link_length * LINK_SEGMENTS_PER_LENGTH), 1);
    return renderable;
}

inline bool is_mouse_hovering_near_link(
    const ImVec2& p0,
    const ImVec2& p1,
    const ImVec2& p2,
    const ImVec2& p3)
{
    bool near = false;
    ImVec2 mouse_pos = ImGui::GetIO().MousePos;

    // First, do an AABB test to see whether it the distance
    // to the line is worth checking in greater detail
    float xmin = ImMin(p0.x, p3.x);
    float xmax = ImMax(p0.x, p3.x);
    float ymin = ImMin(p0.y, p3.y);
    float ymax = ImMax(p0.y, p3.y);

    if ((mouse_pos.x > xmin && mouse_pos.x < xmax) &&
        (mouse_pos.y > ymin && mouse_pos.y < ymax))
    {
        float distance =
            get_distance_to_cubic_bezier(mouse_pos, p0, p1, p2, p3);
        if (distance < LINK_HOVER_DISTANCE)
        {
            near = true;
        }
    }

    return near;
}

inline ImVec2 input_pin_position(
    const ImRect& node_rect,
    const ImRect& attr_rect)
{
    return ImVec2(node_rect.Min.x, 0.5f * (attr_rect.Min.y + attr_rect.Max.y));
}

inline ImVec2 output_pin_position(
    const ImRect& node_rect,
    const ImRect& attr_rect)
{
    return ImVec2(node_rect.Max.x, 0.5f * (attr_rect.Min.y + attr_rect.Max.y));
}

inline bool is_pin_valid(const Pin& pin)
{
    return pin.node_idx != INVALID_INDEX &&
           pin.attribute_idx != INVALID_INDEX && pin.type != AttributeType_None;
}
} // namespace

struct EditorContext
{
    ImGuiStorage node_map;
    ImVector<Node> nodes;
    ImVector<ImGuiID> keys;
    ImVector<Link> links;

    struct
    {
        int current_index;
        ImVector<Event> events;
    } event_queue;

    // ui related fields
    ImVec2 panning;
    ImDrawList* grid_draw_list;

    EditorContext()
        : node_map(), nodes(), links(), event_queue(), panning(0.f, 0.f),
          grid_draw_list(nullptr)
    {
    }
};

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

void EditorContextSet(EditorContext* ctx) { g.editor_ctx = ctx; }

namespace
{
inline int node_index_to_id(int node_idx)
{
    return editor_context_get().nodes[node_idx].id;
}

int find_or_create_new_node(int id)
{
    EditorContext& editor = editor_context_get();

    int idx = editor.node_map.GetInt(ImGuiID(id), INVALID_INDEX);
    if (idx == INVALID_INDEX)
    {
        idx = editor.nodes.size();
        editor.nodes.push_back(Node());
        editor.keys.push_back(ImGuiID(id));
        assert(idx != INVALID_INDEX);
        // TODO: set more new node id state here?
        editor.nodes[idx].id = ImGuiID(id);
        editor.node_map.SetInt(ImGuiID(id), idx);
    }
    return idx;
}

bool link_exists(const Pin& start, const Pin& end)
{
    const EditorContext& editor = editor_context_get();
    for (int i = 0u; i < editor.links.size(); i++)
    {
        const Link& link = editor.links[i];
        if (link.pin1 == start && link.pin2 == end)
            return true;
    }

    return false;
}

inline Event event_for_link_deleted(const Link& link)
{
    const Pin& output_pin =
        link.pin1.type == AttributeType_Output ? link.pin1 : link.pin2;
    const Pin& input_pin =
        link.pin1.type == AttributeType_Input ? link.pin1 : link.pin2;

    Event event;
    event.type = EventType_LinkDeleted;
    event.link_deleted.output_node = output_pin.node_idx;
    event.link_deleted.output_attribute = output_pin.attribute_idx;
    event.link_deleted.input_node = input_pin.node_idx;
    event.link_deleted.input_attribute = input_pin.attribute_idx;

    return event;
}

void draw_grid(const EditorContext& editor)
{
    const ImVec2 offset = editor.panning;
    const ImVec2 canvas_size = ImGui::GetWindowSize();
    for (float x = fmodf(offset.x, GRID_SIZE); x < canvas_size.x;
         x += GRID_SIZE)
    {
        editor.grid_draw_list->AddLine(
            editor_space_to_screen_space(ImVec2(x, 0.0f)),
            editor_space_to_screen_space(ImVec2(x, canvas_size.y)),
            g.color_styles[ColorStyle_GridLine]);
    }
    for (float y = fmodf(offset.y, GRID_SIZE); y < canvas_size.y;
         y += GRID_SIZE)
    {
        editor.grid_draw_list->AddLine(
            editor_space_to_screen_space(ImVec2(0.0f, y)),
            editor_space_to_screen_space(ImVec2(canvas_size.x, y)),
            g.color_styles[ColorStyle_GridLine]);
    }
}

void draw_pins(
    const EditorContext& editor,
    int node_idx,
    AttributeType attribute_type,
    Pin& pin_hovered)
{
    const Node& node = editor.nodes[node_idx];
    assert(
        attribute_type == AttributeType_Input ||
        attribute_type == AttributeType_Output);
    const ImVector<ImRect>& attributes = attribute_type == AttributeType_Input
                                             ? node.input_attributes
                                             : node.output_attributes;
    const ImRect node_rect = get_node_rect(node);
    for (int idx = 0u; idx < attributes.size(); idx++)
    {
        ImVec2 pin_pos = attribute_type == AttributeType_Input
                             ? input_pin_position(node_rect, attributes[idx])
                             : output_pin_position(node_rect, attributes[idx]);
        if (is_mouse_hovering_near_point(pin_pos, NODE_PIN_HOVER_RADIUS))
        {
            const Pin& link_start = g.link_dragged.pin1;
            bool hovered_pin_is_valid = true;
            if (is_pin_valid(link_start))
            {
                // Ensure that the pin is not linking to the same node, and that
                // the link is output
                // -> input.
                // Also ensure that the link doesn't already exist
                hovered_pin_is_valid =
                    link_start.node_idx != node_idx &&
                    link_start.type != attribute_type &&
                    !link_exists(
                        link_start, Pin(node_idx, idx, attribute_type));
            }

            if (hovered_pin_is_valid)
            {
                editor.grid_draw_list->AddCircleFilled(
                    pin_pos,
                    NODE_PIN_RADIUS,
                    node.color_styles[ColorStyle_PinHovered]);
                pin_hovered = Pin(node_idx, idx, attribute_type);
            }
            else
            {
                editor.grid_draw_list->AddCircleFilled(
                    pin_pos,
                    NODE_PIN_RADIUS,
                    node.color_styles[ColorStyle_PinInvalid]);
            }
        }
        else
        {
            editor.grid_draw_list->AddCircleFilled(
                pin_pos, NODE_PIN_RADIUS, node.color_styles[ColorStyle_Pin]);
        }
        editor.grid_draw_list->AddCircle(
            pin_pos, NODE_PIN_RADIUS, node.color_styles[ColorStyle_PinOutline]);
    }
}

void draw_node(const EditorContext& editor, int node_idx, Pin& pin_hovered)
{
    const Node& node = editor.nodes[node_idx];
    ImGui::PushID(node.id);

    editor.grid_draw_list->ChannelsSetCurrent(CHANNEL_FOREGROUND);

    const ImRect node_rect = get_node_rect(node);

    ImGui::SetCursorPos(node.origin + editor.panning);
    ImGui::InvisibleButton(node.name, node_rect.GetSize());

    if (ImGui::IsItemHovered())
    {
        g.hovered_node = node_idx;
        if (ImGui::IsMouseClicked(0))
        {
            g.selected_node = g.hovered_node;
        }
    }

    // Check to see whether the node moved during the frame. The node's position
    // is updated after the node has been drawn (because the used has already
    // rendered the UI!). Also check to see that the
    if (ImGui::IsItemActive() && pin_hovered.node_idx != node_idx &&
        ImGui::IsMouseDragging(0))
    {
        g.moved_node.index = node_idx;
        g.moved_node.position = node.origin + ImGui::GetIO().MouseDelta;
    }

    ColorStyle color_style_node = ColorStyle_NodeBackground;
    ColorStyle color_style_title = ColorStyle_TitleBar;

    if (g.selected_node == node_idx)
    {
        color_style_node = ColorStyle_NodeBackgroundSelected;
        color_style_title = ColorStyle_TitleBarSelected;
    }
    else if (g.hovered_node == node_idx)
    {
        color_style_node = ColorStyle_NodeBackgroundHovered;
        color_style_title = ColorStyle_TitleBarHovered;
    }

    {
        // node base
        editor.grid_draw_list->AddRectFilled(
            node_rect.Min,
            node_rect.Max,
            node.color_styles[color_style_node],
            NODE_CORNER_ROUNDNESS);

        // title bar:
        {
            ImRect title_rect = get_title_bar_rect(node);
            // TODO: better consistency here?
            // why subtract panning from Min, and then subtract it only from the
            // y-component of Max? it's because Max.x is computed from the node
            // rect, which already has the panning subtracted from it! the
            // y-component is just looked up from the text height
            title_rect.Min += editor.panning;
            title_rect.Max.y += editor.panning.y;
            editor.grid_draw_list->AddRectFilled(
                title_rect.Min,
                title_rect.Max,
                node.color_styles[color_style_title],
                NODE_CORNER_ROUNDNESS,
                ImDrawCornerFlags_Top);
            ImGui::SetCursorPos(get_node_title_origin(node) + editor.panning);
            ImGui::PushItemWidth(title_rect.Max.x - title_rect.Min.x);
            ImGui::TextUnformatted(node.name);
            ImGui::PopItemWidth();
        }
        // outline:
        editor.grid_draw_list->AddRect(
            node_rect.Min,
            node_rect.Max,
            node.color_styles[ColorStyle_NodeOutline],
            NODE_CORNER_ROUNDNESS);
    }

    // input & output attribute pins:
    draw_pins(editor, node_idx, AttributeType_Input, pin_hovered);
    draw_pins(editor, node_idx, AttributeType_Output, pin_hovered);

    ImGui::PopID();
}

void draw_link(const EditorContext& editor, const int link_idx)
{
    const Pin& pin_input =
        editor.links[link_idx].pin1.type == AttributeType_Input
            ? editor.links[link_idx].pin1
            : editor.links[link_idx].pin2;
    const Pin& pin_output =
        editor.links[link_idx].pin1.type == AttributeType_Output
            ? editor.links[link_idx].pin1
            : editor.links[link_idx].pin2;

    ImVec2 start, end;
    {
        // TODO: maybe compute the current node_rect and store it instead of
        // recomputing it all the time
        const ImRect node_rect =
            get_node_rect(editor.nodes[pin_output.node_idx]);
        start = output_pin_position(
            node_rect,
            editor.nodes[pin_output.node_idx]
                .output_attributes[pin_output.attribute_idx]);
    }

    {
        const ImRect node_rect =
            get_node_rect(editor.nodes[pin_input.node_idx]);
        end = input_pin_position(
            node_rect,
            editor.nodes[pin_input.node_idx]
                .input_attributes[pin_input.attribute_idx]);
    }

    const LinkRenderable link_renderable = get_link_renderable(start, end);

    const bool is_hovered = is_mouse_hovering_near_link(
        link_renderable.p0,
        link_renderable.p1,
        link_renderable.p2,
        link_renderable.p3);
    if (is_hovered)
    {
        g.hovered_link = link_idx;
        if (ImGui::IsMouseClicked(0))
        {
            g.selected_link = link_idx;
        }
    }

    ColorStyle style = ColorStyle_Link;
    if (g.selected_link == link_idx)
    {
        style = ColorStyle_LinkSelected;
    }
    else if (is_hovered)
    {
        style = ColorStyle_LinkHovered;
    }

    const ImU32 link_color = g.color_styles[style];

    editor.grid_draw_list->AddBezierCurve(
        link_renderable.p0,
        link_renderable.p1,
        link_renderable.p2,
        link_renderable.p3,
        link_color,
        LINK_THICKNESS,
        link_renderable.num_segments);
}
} // namespace

void Initialize()
{
    assert(g.guard.initialized == false);
    g.guard.initialized = true;

    g.default_editor_ctx = NULL;
    g.editor_ctx = NULL;
    g.grid_origin = ImVec2(0.0f, 0.0f);
    g.current_scope = SCOPE_NONE;

    g.default_editor_ctx = EditorContextCreate();
    EditorContextSet(g.default_editor_ctx);

    g.current_node.index = INVALID_INDEX;
    g.current_node.attribute.type = AttributeType_None;
    g.current_node.attribute.index = INVALID_INDEX;

    g.link_dragged = Link();

    g.hovered_node = INVALID_INDEX;
    g.selected_node = INVALID_INDEX;
    g.hovered_link = INVALID_INDEX;
    g.selected_link = INVALID_INDEX;

    g.color_styles[ColorStyle_NodeBackground] = IM_COL32(60, 60, 60, 255);
    g.color_styles[ColorStyle_NodeBackgroundHovered] =
        IM_COL32(75, 75, 75, 255);
    g.color_styles[ColorStyle_NodeBackgroundSelected] =
        IM_COL32(75, 75, 75, 255); // TODO: different default value for this?
    g.color_styles[ColorStyle_NodeOutline] = IM_COL32(100, 100, 100, 255);
    g.color_styles[ColorStyle_TitleBar] = IM_COL32(60, 0, 40, 255);
    g.color_styles[ColorStyle_TitleBarHovered] = IM_COL32(85, 0, 55, 255);
    g.color_styles[ColorStyle_TitleBarSelected] =
        IM_COL32(85, 0, 55, 255); // TODO: different default value for this?
    g.color_styles[ColorStyle_Link] = IM_COL32(200, 200, 100, 255);
    g.color_styles[ColorStyle_LinkHovered] = IM_COL32(250, 250, 100, 255);
    g.color_styles[ColorStyle_LinkSelected] = IM_COL32(255, 255, 255, 255);
    g.color_styles[ColorStyle_Pin] = IM_COL32(150, 150, 150, 255);
    g.color_styles[ColorStyle_PinHovered] = IM_COL32(200, 200, 100, 255);
    g.color_styles[ColorStyle_PinInvalid] = IM_COL32(255, 0, 0, 255);
    g.color_styles[ColorStyle_PinOutline] = IM_COL32(200, 200, 200, 255);
    g.color_styles[ColorStyle_GridBackground] = IM_COL32(40, 40, 50, 200);
    g.color_styles[ColorStyle_GridLine] = IM_COL32(200, 200, 200, 40);
}

void Shutdown() { EditorContextFree(g.default_editor_ctx); }

void BeginNodeEditor()
{
    // Remember to call Initialize() before calling BeginNodeEditor()
    assert(g.guard.initialized);
    assert(g.current_scope == SCOPE_NONE);
    g.current_scope = SCOPE_EDITOR;

    // Reset state from previous pass

    g.active_node.index = INVALID_INDEX;
    g.active_node.attribute = INVALID_INDEX;
    g.moved_node.index = INVALID_INDEX;
    g.moved_node.position = ImVec2(0.0f, 0.0f);
    g.hovered_node = INVALID_INDEX;
    g.hovered_link = INVALID_INDEX;
    // reset ui events for the current editor context
    EditorContext& editor = editor_context_get();
    editor.event_queue.current_index = 0;
    editor.event_queue.events.clear();

    assert(editor.grid_draw_list == nullptr);

    ImGui::BeginGroup();
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::PushStyleColor(
            ImGuiCol_ChildWindowBg, g.color_styles[ColorStyle_GridBackground]);
        ImGui::BeginChild(
            "scrolling_region",
            ImVec2(0.f, 0.f),
            true,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
        g.grid_origin = ImGui::GetCursorScreenPos();
        // prepare for layering the node content on top of the nodes
        // NOTE: the draw list has to be captured here, because we want all the
        // content to clip the scrolling_region child window.
        editor.grid_draw_list = ImGui::GetWindowDrawList();
        editor.grid_draw_list->ChannelsSplit(CHANNEL_COUNT);
        editor.grid_draw_list->ChannelsSetCurrent(CHANNEL_BACKGROUND);

        // TODO: showing the grid should be a setting
        if (/*show_grid*/ true)
        {
            draw_grid(editor);
        }
    }

    editor.grid_draw_list->ChannelsSetCurrent(CHANNEL_UI);
}

void EndNodeEditor()
{
    assert(g.current_scope == SCOPE_EDITOR);
    g.current_scope = SCOPE_NONE;

    EditorContext& editor = editor_context_get();

    const bool is_mouse_clicked = ImGui::IsMouseClicked(0);

    /***
     *         ___          __                         __
     *     ___/ (_)__ ___  / /__ ___ __  ___  ___  ___/ /__ ___
     *    / _  / (_-</ _ \/ / _ `/ // / / _ \/ _ \/ _  / -_|_-<
     *    \_,_/_/___/ .__/_/\_,_/\_, / /_//_/\___/\_,_/\__/___/
     *             /_/          /___/
     */

    Pin pin_hovered = Pin();

    for (int node_idx = 0; node_idx < editor.nodes.size(); node_idx++)
    {
        // TODO: perhaps consider putting this inside a BeginChild()  block
        // to see if that would solve the overlapping node problem
        draw_node(editor, node_idx, pin_hovered);
    }

    // check to see if the mouse was near any node
    if (g.hovered_node == INVALID_INDEX)
    {
        if (is_mouse_clicked)
        {
            g.selected_node = INVALID_INDEX;
        }
    }

    /***
     *         ___          __            ___      __
     *     ___/ (_)__ ___  / /__ ___ __  / (_)__  / /__ ___
     *    / _  / (_-</ _ \/ / _ `/ // / / / / _ \/  '_/(_-<
     *    \_,_/_/___/ .__/_/\_,_/\_, / /_/_/_//_/_/\_\/___/
     *             /_/          /___/
     */

    editor.grid_draw_list->ChannelsSetCurrent(CHANNEL_BACKGROUND);

    for (int i = 0; i < editor.links.size(); i++)
    {
        draw_link(editor, i);
    }

    if (g.hovered_link == INVALID_INDEX)
    {
        if (is_mouse_clicked)
        {
            g.selected_link = INVALID_INDEX;
        }
    }

    // See if a new link is created on a pin

    if (is_mouse_clicked)
    {
        g.link_dragged.pin1 = pin_hovered;
    }

    // Render the link being dragged

    if (ImGui::IsMouseDown(0) && is_pin_valid(g.link_dragged.pin1))
    {
        const Pin& pin = g.link_dragged.pin1;
        const Node& node = editor_context_get().nodes[pin.node_idx];
        const ImRect node_rect = get_node_rect(node);
        ImVec2 output_pos, input_pos;
        assert(
            pin.type == AttributeType_Input ||
            pin.type == AttributeType_Output);
        if (pin.type == AttributeType_Input)
        {
            input_pos = input_pin_position(
                node_rect, node.input_attributes[pin.attribute_idx]);
            output_pos = ImGui::GetIO().MousePos;
        }
        else if (pin.type == AttributeType_Output)
        {
            output_pos = output_pin_position(
                node_rect, node.output_attributes[pin.attribute_idx]);
            input_pos = ImGui::GetIO().MousePos;
        }
        const LinkRenderable link_renderable =
            get_link_renderable(output_pos, input_pos);
        editor.grid_draw_list->AddBezierCurve(
            link_renderable.p0,
            link_renderable.p1,
            link_renderable.p2,
            link_renderable.p3,
            g.color_styles[ColorStyle_Link],
            LINK_THICKNESS,
            link_renderable.num_segments);
    }

    // Finish the new link!

    if (ImGui::IsMouseReleased(0))
    {
        // Ensure that a link was actually started before pushing anything into
        // the vector.
        //
        // It's possible to hover over a pin without starting a link by clicking
        // & dragging on a node. The node is moved with a frame delay -- so the
        // mouse might be hovering over a pin when it's released resulting in
        // pin1 never being assigned to a valid node.
        if (is_pin_valid(g.link_dragged.pin1))
        {
            if (is_pin_valid(pin_hovered))
            {
                g.link_dragged.pin2 = pin_hovered;
                assert(
                    g.link_dragged.pin1.type == AttributeType_Input ||
                    g.link_dragged.pin1.type == AttributeType_Output);
                assert(
                    g.link_dragged.pin2.type == AttributeType_Input ||
                    g.link_dragged.pin2.type == AttributeType_Output);
                assert(g.link_dragged.pin1.type != g.link_dragged.pin2.type);
                editor.links.push_back(g.link_dragged);

                const Pin& output_pin =
                    g.link_dragged.pin1.type == AttributeType_Output
                        ? g.link_dragged.pin1
                        : g.link_dragged.pin2;
                const Pin& input_pin =
                    g.link_dragged.pin1.type == AttributeType_Input
                        ? g.link_dragged.pin1
                        : g.link_dragged.pin2;
                Event event;
                event.type = EventType_LinkCreated;
                event.link_created.output_node = output_pin.node_idx;
                event.link_created.output_attribute = output_pin.attribute_idx;
                event.link_created.input_node = input_pin.node_idx;
                event.link_created.input_attribute = input_pin.attribute_idx;
                editor.event_queue.events.push_back(event);

                // finally, reset the newly created link
                g.link_dragged = Link();
            }
            else
            {
                g.link_dragged.pin1 = Pin();
            }
        }
    }

    /***
     *       ____      _     __
     *      / _(_)__  (_)__ / /
     *     / _/ / _ \/ (_-</ _ \
     *    /_//_/_//_/_/___/_//_/
     *
     */

    if (g.moved_node.index != INVALID_INDEX &&
        g.link_dragged.pin1.node_idx == INVALID_INDEX)
    {
        // Don't move the node if we're dragging a link
        editor.nodes[g.moved_node.index].origin = g.moved_node.position;
    }

    // set channel 0 before merging, or else UI rendering is broken
    editor.grid_draw_list->ChannelsSetCurrent(0);
    editor.grid_draw_list->ChannelsMerge();
    editor.grid_draw_list = nullptr;

    // see if the user deleted a node or link
    if (ImGui::IsKeyReleased(SDL_SCANCODE_DELETE))
    {
        if (g.selected_node != INVALID_INDEX)
        {
            assert(g.selected_node >= 0);
            assert(g.selected_node < editor.nodes.size());
            // TODO: maybe this could be a part of the global context
            ImVector<int> deleted_links;
            // Update the link array
            for (int i = 0; i < editor.links.size(); i++)
            {
                Link& link = editor.links[i];
                // Remove the links that are connectedg to the deleted node
                if (link.pin1.node_idx == g.selected_node ||
                    link.pin2.node_idx == g.selected_node)
                {
                    Event event = event_for_link_deleted(link);
                    editor.event_queue.events.push_back(event);
                    deleted_links.push_back(i);
                }
                // Update the node indices of the links, since the node gets
                // swap-removed
                if (link.pin1.node_idx == editor.nodes.size() - 1)
                {
                    link.pin1.node_idx = g.selected_node;
                }
                if (link.pin2.node_idx == editor.nodes.size() - 1)
                {
                    link.pin2.node_idx = g.selected_node;
                }
            }
            // Clean up the links array.
            // Sort the indices in descending order so that we start with the
            // largest index first. That way we can swap-remove each link
            // without invalidating any indices.
            std::sort(
                deleted_links.begin(),
                deleted_links.end(),
                std::greater<int>());
            for (int i = 0; i < deleted_links.size(); i++)
            {
                int idx = deleted_links[i];
                editor.links.erase_unsorted(editor.links.Data + idx);
            }

            // construct and queue the event
            Event event;
            event.type = EventType_NodeDeleted;
            event.node_deleted.node_idx = node_index_to_id(g.selected_node);
            editor.event_queue.events.push_back(event);

            // erase the node id from the map
            editor.node_map.SetInt(editor.nodes.back().id, g.selected_node);
            editor.node_map.SetInt(INVALID_INDEX, g.selected_node);

            // erase the entry from the array
            editor.nodes.erase_unsorted(editor.nodes.Data + g.selected_node);

            g.selected_node = INVALID_INDEX;
        }

        else if (g.selected_link != INVALID_INDEX)
        {
            assert(g.selected_link >= 0);
            assert(g.selected_link < editor.links.size());

            const Link link = editor.links[g.selected_link];
            Event event = event_for_link_deleted(link);
            editor.event_queue.events.push_back(event);

            editor.links.erase_unsorted(editor.links.Data + g.selected_link);

            g.selected_link = INVALID_INDEX;
        }
    }

    // apply panning if the mouse was dragged
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() &&
        ImGui::IsMouseDragging(2, 0))
    {
        editor.panning = editor.panning + ImGui::GetIO().MouseDelta;
    }

    // pop style
    ImGui::EndChild();      // end scrolling region
    ImGui::PopStyleColor(); // pop child window background color
    ImGui::PopStyleVar();   // pop window padding
    ImGui::PopStyleVar();   // pop frame padding
    ImGui::EndGroup();

    for (int idx = 0; idx < editor.nodes.size(); idx++)
    {
        Node& node = editor.nodes[idx];

        node.input_attributes.clear();
        node.output_attributes.clear();
    }
}

void BeginNode(int node_id)
{
    // Remember to call BeginNodeEditor before calling BeginNode
    assert(g.current_scope == SCOPE_EDITOR);
    g.current_scope = SCOPE_NODE;
    assert(g.current_node.index == INVALID_INDEX);

    EditorContext& editor = editor_context_get();

    {
        int idx = find_or_create_new_node(node_id);
        g.current_node.index = idx;
        memcpy(
            editor.nodes[idx].color_styles,
            g.color_styles,
            sizeof(g.color_styles));
    }

    ImGui::SetCursorPos(
        get_node_content_origin(editor.nodes[g.current_node.index]) +
        editor.panning);

    ImGui::PushID(node_id);
    ImGui::BeginGroup();
}

void Name(const char* name)
{
    assert(g.current_scope == SCOPE_NODE);

    Node& node = editor_context_get().nodes[g.current_node.index];
    assert(strlen(name) < NODE_NAME_STR_LEN);
    memset(node.name, 0, NODE_NAME_STR_LEN);
    memcpy(node.name, name, strlen(name));
}

void EndNode()
{
    assert(g.current_scope == SCOPE_NODE);
    g.current_scope = SCOPE_EDITOR;

    EditorContext& editor = editor_context_get();

    ImGui::EndGroup();
    ImGui::PopID();
    editor.nodes[g.current_node.index].content_rect = get_item_rect();
    g.current_node.index = INVALID_INDEX;
}

int BeginAttribute(int id, AttributeType type)
{
    // Make sure to call BeginNode() before calling
    // BeginAttribute()
    assert(g.current_scope == SCOPE_NODE);
    g.current_scope = SCOPE_ATTRIBUTE;

    ImGui::BeginGroup();
    ImGui::PushID(id);

    g.current_node.attribute.type = type;

    switch (type)
    {
        case AttributeType_Input:
            g.current_node.attribute.index = editor_context_get()
                                                 .nodes[g.current_node.index]
                                                 .input_attributes.size();
        case AttributeType_Output:
            g.current_node.attribute.index = editor_context_get()
                                                 .nodes[g.current_node.index]
                                                 .output_attributes.size();
        default:
            g.current_node.attribute.index = INVALID_INDEX;
    }

    return g.current_node.attribute.index;
}

void EndAttribute()
{
    assert(g.current_scope == SCOPE_ATTRIBUTE);
    g.current_scope = SCOPE_NODE;

    ImGui::PopID();
    ImGui::EndGroup();

    if (ImGui::IsItemActive())
    {
        g.active_node.index = g.current_node.index;
        g.active_node.attribute = g.current_node.attribute.index;
    }

    Node& current_node = editor_context_get().nodes[g.current_node.index];

    if (g.current_node.attribute.type == AttributeType_Input)
    {
        current_node.input_attributes.push_back(get_item_rect());
    }
    else if (g.current_node.attribute.type == AttributeType_Output)
    {
        current_node.output_attributes.push_back(get_item_rect());
    }
}

void PushColorStyle(ColorStyle item, ImU32 color)
{
    assert(g.guard.initialized);
    g.color_style_stack.push_back(g.color_styles[item]);
    g.color_styles[item] = color;
}

void PopColorStyle(ColorStyle item)
{
    assert(g.color_style_stack.size() > 0);
    g.color_styles[item] = g.color_style_stack.back();
    g.color_style_stack.pop_back();
}

void SetNodePos(
    int node_id,
    const ImVec2& screen_space_pos,
    ImGuiCond condition)
{
    assert(g.guard.initialized);
    int index = find_or_create_new_node(node_id);
    editor_context_get().nodes[index].origin =
        screen_space_pos - editor_context_get().panning - g.grid_origin;
}

bool IsAttributeActive(int* node, int* attribute)
{
    assert(g.guard.initialized);
    if (g.active_node.index != INVALID_INDEX)
    {
        // TODO: what if the pointers are null?
        *node = g.active_node.index;
        *attribute = g.active_node.attribute;

        return true;
    }

    return false;
}

bool PollEvent(Event& event)
{
    EditorContext& editor = editor_context_get();
    if (editor.event_queue.current_index < editor.event_queue.events.size())
    {
        event = editor.event_queue.events[editor.event_queue.current_index++];
        return true;
    }

    return false;
}

namespace
{
void node_line_handler(EditorContext& editor, const char* line)
{
    int i;
    float x, y;
    if (sscanf(line, "[node.%i", &i) == 1)
    {
        editor.nodes.back().id = i;
        editor.node_map.SetInt(ImGuiID(i), editor.nodes.size() - 1);
    }
    else if (sscanf(line, "origin=%f,%f", &x, &y) == 2)
    {
        editor.nodes.back().origin = ImVec2(x, y);
    }
}

void link_line_handler(EditorContext& editor, const char* line)
{
    // link header isn't parsed for now
    Pin pin;
    if (sscanf(line, "output=%i,%i", &pin.node_idx, &pin.attribute_idx))
    {
        pin.type = AttributeType_Output;
        editor.links.back().pin1 = pin;
    }
    else if (sscanf(line, "input=%i,%i", &pin.node_idx, &pin.attribute_idx))
    {
        pin.type = AttributeType_Input;
        editor.links.back().pin2 = pin;
    }
}
} // namespace

const char* SaveEditorStateToMemory(const EditorContext* editor_ptr)
{
    const EditorContext& editor =
        editor_ptr == NULL ? editor_context_get() : *editor_ptr;

    g.text_buffer.clear();
    // TODO: check to make sure that the estimate is the upper bound of element
    g.text_buffer.reserve(64 * editor.nodes.size() + 64 * editor.links.size());

    // TODO: save editor panning to disk as well

    for (int i = 0; i < editor.nodes.size(); i++)
    {
        const Node& node = editor.nodes[i];
        g.text_buffer.appendf("\n[node.%d]\n", node.id);
        g.text_buffer.appendf(
            "origin=%i,%i\n", (int)node.origin.x, (int)node.origin.y);
    }

    for (int i = 0; i < editor.links.size(); i++)
    {
        const Link& link = editor.links[i];
        const Pin& output =
            link.pin1.type == AttributeType_Output ? link.pin1 : link.pin2;
        const Pin& input =
            link.pin1.type == AttributeType_Input ? link.pin1 : link.pin2;

        // links don't have a unique id for now, so just use the index
        g.text_buffer.appendf("\n[link.%d]\n", i);
        g.text_buffer.appendf(
            "output=%d,%d\n", output.node_idx, output.attribute_idx);
        g.text_buffer.appendf(
            "input=%d,%d\n", input.node_idx, input.attribute_idx);
    }

    return g.text_buffer.c_str();
}

void LoadEditorStateFromMemory(
    const char* data,
    size_t data_size,
    EditorContext* editor_ptr)
{
    if (data_size == 0u)
    {
        return;
    }

    EditorContext& editor =
        editor_ptr == NULL ? editor_context_get() : *editor_ptr;

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
                editor.nodes.push_back(Node());
                line_handler = node_line_handler;
            }
            else if (strncmp(line + 1, "link", 4) == 0)
            {
                editor.links.push_back(Link());
                line_handler = link_line_handler;
            }
            else if (strcmp(line + 1, "editor") == 0)
            {
                // TODO editor param handling
            }
        }

        if (line_handler != NULL)
        {
            line_handler(editor, line);
        }
    }
    ImGui::MemFree(buf);
}
} // namespace imnodes
