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
    ColorStyle_LinkHovered,
    ColorStyle_LinkSelected,
    ColorStyle_Pin,
    ColorStyle_PinHovered,
    ColorStyle_PinInvalid,
    ColorStyle_PinOutline,
    ColorStyle_GridBackground,
    ColorStyle_GridLine,
    ColorStyle_Count
};

enum EventType
{
    EventType_LinkCreated,
    EventType_NodeDeleted,
    EventType_LinkDeleted
};

struct Event
{
    EventType type;
    union
    {
        struct
        {
            int output_node;
            int output_attribute;
            int input_node;
            int input_attribute;
        } link_created;

        struct
        {
            int node_idx;
        } node_deleted;

        struct
        {
            int output_node;
            int output_attribute;
            int input_node;
            int input_attribute;
        } link_deleted;
    };
};

struct EditorContext;

EditorContext* EditorContextCreate();
void EditorContextFree(EditorContext*);
void EditorContextSet(EditorContext*);

void Initialize();
void Shutdown();

void BeginNodeEditor();

void PushColorStyle(ColorStyle item, ImU32 color);

void BeginNode(int id);
void Name(const char* name);

int BeginAttribute(int id, AttributeType type);
void EndAttribute();

void EndNode();

void PopColorStyle(ColorStyle item);

void EndNodeEditor();

// TODO: condition is unused
void SetNodePos(int node_id, const ImVec2& pos, ImGuiCond condition);

bool IsAttributeActive(int* node, int* attribute);

bool PollEvent(Event& event);

// Save the editor state to a string. The data is written in the TOML format.
// If the editor is left out, then the function will save the currently set
// editor's state.
const char* SaveEditorStateToMemory(const EditorContext* editor = NULL);
void LoadEditorStateFromMemory(
    const char* data,
    size_t data_size,
    EditorContext* editor = NULL);
} // namespace imnodes
