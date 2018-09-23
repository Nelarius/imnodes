#pragma once

#include "imgui.h"

namespace narwhal
{
enum AttributeType
{
    AttributeType_None,
    AttributeType_Input,
    AttributeType_Output,
    AttributeType_Internal
};

enum ColorStyle
{
    ColorStyle_NodeBackground = 0,
    ColorStyle_NodeBackgroundHovered,
    ColorStyle_NodeBackgroundSelected,
    ColorStyle_NodeOutline,
    ColorStyle_TitleBar,
    ColorStyle_TitleBarHovered,
    ColorStyle_TitleBarSelected,
    ColorStyle_Link,
    ColorStyle_Pin,
    ColorStyle_PinHovered,
    ColorStyle_PinInvalid,
    ColorStyle_PinOutline,
    ColorStyle_GridBackground,
    ColorStyle_GridLine,
    ColorStyle_Count
};

struct EditorContext;

EditorContext* editor_context_create();
void editor_context_free(EditorContext*);
void editor_context_set(EditorContext*);

void initialize();
void shutdown();

void begin_node_editor();

void begin_node(int id);
void name(const char* name);

int begin_attribute(int id, AttributeType type);
void end_attribute();

void end_node();

void end_node_editor();

void push_color_style(ColorStyle item, ImU32 color);
void pop_color_style(ColorStyle item);

// TODO: is there a need for manually controlling which links are shown?

void set_node_pos(int node_id, const ImVec2& pos, ImGuiCond condition);

bool is_attribute_active(int* node, int* attribute);

bool new_link_created(
    int* output_node,
    int* output_attribute,
    int* input_node,
    int* input_attribute);

bool node_deleted(int* deleted_node);
bool link_deleted(int* output_node, int* output_attribute, int* input_node, int* input_attribute);
}
