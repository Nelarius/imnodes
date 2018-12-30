#pragma once

#include "imgui.h"
#include <stddef.h>

namespace imnodes
{
enum AttributeType
{
    AttributeType_None,
    AttributeType_Input,
    AttributeType_Output
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

void PushColorStyle(ColorStyle item, ImU32 color);

void BeginNode(int id);
void Name(const char* name);

// The attribute ids must be unique with regards to other attribute ids.
void BeginAttribute(int id, AttributeType type);
void EndAttribute();

void EndNode();

// The attributes ids used here must match the ids used in BeginAttribute()
void Link(int id, int start_attr, int end_attr);

void PopColorStyle();

void EndNodeEditor();

// TODO: condition is unused
void SetNodePos(int node_id, const ImVec2& pos, ImGuiCond condition);

// new replacements for events
bool IsNodeHovered(int* node_id);
bool IsLinkHovered(int* link_id);
bool IsPinHovered(int* attribute_id);

bool IsNodeSelected(int* node_id);
bool IsLinkSelected(int* link_id);

// Is the user dragging a new link?
bool IsLinkStarted(int* started_at_attr);
// Did the user drop the new link before connecting it to a second attribute?
bool IsLinkDropped();
// Did the user create a new link?
bool IsLinkCreated(int* started_at_attr, int* ended_at_attr);

// Save the editor state to a string. The data is written in the INI format.
// If the editor is left out, then the function will save the currently set
// editor's state.
const char* SaveCurrentEditorStateToMemory(size_t* data_size = NULL);
const char* SaveEditorStateToMemory(
    const EditorContext* editor,
    size_t* data_size = NULL);

void LoadCurrentEditorStateFromMemory(const char* data, size_t data_size);
void LoadEditorStateFromMemory(
    EditorContext* editor,
    const char* data,
    size_t data_size);

void SaveCurrentEditorStateToDisk(const char* file_name);
void SaveEditorStateToDisk(const EditorContext* editor, const char* file_name);

void LoadCurrentEditorStateFromDisk(const char* file_name);
void LoadEditorStateFromDisk(EditorContext* editor, const char* file_name);

} // namespace imnodes
