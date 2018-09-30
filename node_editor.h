#pragma once

#include "imgui.h"

namespace imnodes
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

EditorContext* EditorContextCreate();
void EditorContextFree(EditorContext*);
void EditorContextSet(EditorContext*);

void Initialize();
void Shutdown();

void BeginNodeEditor();

void BeginNode(int id);
void Name(const char* name);

int BeginAttribute(int id, AttributeType type);
void EndAttribute();

void EndNode();

void EndNodeEditor();

void PushColorStyle(ColorStyle item, ImU32 color);
void PopColorStyle(ColorStyle item);

// TODO: condition is unused
void SetNodePos(int node_id, const ImVec2& pos, ImGuiCond condition);

bool IsAttributeActive(int* node, int* attribute);

bool NewLinkCreated(
    int* output_node,
    int* output_attribute,
    int* input_node,
    int* input_attribute);
bool NodeDeleted(int* deleted_node);
bool LinkDeleted(
    int* output_node,
    int* output_attribute,
    int* input_node,
    int* input_attribute);
} // namespace imnodes
