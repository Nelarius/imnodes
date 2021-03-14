<h1 align="center">imnodes</h1>

<p align="center">A small, dependency-free node editor extension for <a href="https://github.com/ocornut/imgui">dear imgui</a>.</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/Nelarius/imnodes/master/img/imnodes.gif?token=ADH_jEpqbBrw0nH-BUmOip490dyO2CnRks5cVZllwA%3D%3D">
</p>

[![Build Status](https://github.com/nelarius/imnodes/workflows/Build/badge.svg)](https://github.com/nelarius/imnodes/actions?workflow=Build)

Features:

* Create nodes, links, and pins in an immediate-mode style
* Single header and source file, just copy-paste `imnodes.h` and `imnodes.cpp` into your project. The only dependency is `dear imgui` itself!
* Written in the same style of C++ as `dear imgui` itself -- no modern C++ used
* Use regular `dear imgui` widgets inside the nodes
* Multiple node and link selection with a box selector
* Nodes, links, and pins are fully customizable, from color style to layout
* Default themes match `dear imgui`'s default themes

Scroll down for a brief tour, known issues, and further information!

## Build the examples

This repository includes a few example files, under `example/`. You can copy-paste them into your own imgui project, or you can build them here, in this repository. To build here:
* SDL2 needs to be installed
* premake5 is used to generate the build files

```bash
# Assuming you've installed SDL2 via vcpkg, for instance
$ premake5 gmake \
    --sdl-include-path=/Users/nelarius/vcpkg/installed/x64-osx/include/SDL2 \
    --sdl-link-path=/Users/nelarius/vcpkg/installed/x64-osx/lib

# Or alternatively, if you are on MacOS and have the SDL2 framework installed
$ premake5 gmake --use-sdl-framework

$ make all -j
```

## A brief tour

Here is a small overview of how the extension is used. For more information on example usage, scroll to the bottom of the README.

Before anything can be done, the library must be initialized. This can be done at the same time as `dear imgui` initialization.

```cpp
ImGui::CreateContext();
imnodes::CreateContext();

// elsewhere in the code...
imnodes::DestroyContext();
ImGui::DestroyContext();
```

The node editor is a workspace which contains nodes. The node editor must be instantiated within a window, like any other UI element.

```cpp
ImGui::Begin("node editor");

imnodes::BeginNodeEditor();
imnodes::EndNodeEditor();

ImGui::End();
```

Now you should have a workspace with a grid visible in the window. An empty node can now be instantiated:

```cpp
const int hardcoded_node_id = 1;

imnodes::BeginNodeEditor();

imnodes::BeginNode(hardcoded_node_id);
ImGui::Dummy(ImVec2(80.0f, 45.0f));
imnodes::EndNode();

imnode::EndNodeEditor();
```

Nodes, like windows in `dear imgui` must be uniquely identified. But we can't use the node titles for identification, because it should be possible to have many nodes of the same name in the workspace. Instead, you just use integers for identification.

Attributes are the UI content of the node. An attribute will have a pin (the little circle) on either side of the node. There are two types of attributes: input, and output attributes. Input attribute pins are on the left side of the node, and output attribute pins are on the right. Like nodes, pins must be uniquely identified.

```cpp
imnodes::BeginNode(hardcoded_node_id);

const int output_attr_id = 2;
imnodes::BeginOutputAttribute(output_attr_id);
// in between Begin|EndAttribute calls, you can call ImGui
// UI functions
ImGui::Text("output pin");
imnodes::EndOutputAttribute();

imnodes::EndNode();
```

The extension doesn't really care what is in the attribute. It just renders the pin for the attribute, and allows the user to create links between pins.

A title bar can be added to the node using `BeginNodeTitleBar` and `EndNodeTitleBar`. Like attributes, you place your title bar's content between the function calls. Note that these functions have to be called before adding attributes or other `dear imgui` UI elements to the node, since the node's layout is built in order, top-to-bottom.

```cpp
imnodes::BeginNode(hardcoded_node_id);

imnodes::BeginNodeTitleBar();
ImGui::TextUnformatted("output node");
imnodes::EndNodeTitleBar();

// pins and other node UI content omitted...

imnodes::EndNode();
```

The user has to render their own links between nodes as well. A link is a curve which connects two attributes. A link is just a pair of attribute ids. And like nodes and attributes, links too have to be identified by unique integer values:

```cpp
std::vector<std::pair<int, int>> links;
// elsewhere in the code...
for (int i = 0; i < links.size(); ++i)
{
  const std::pair<int, int> p = links[i];
  // in this case, we just use the array index of the link
  // as the unique identifier
  imnodes::Link(i, p.first, p.second);
}
```

After `EndNodeEditor` has been called, you can check if a link was created during the frame with the function call `IsLinkCreated`:

```cpp
int start_attr, end_attr;
if (imnodes::IsLinkCreated(&start_attr, &end_attr))
{
  links.push_back(std::make_pair(start_attr, end_attr));
}
```

In addition to checking for new links, you can also check whether UI elements are being hovered over by the mouse cursor:

```cpp
int node_id;
if (imnodes::IsNodeHovered(&node_id))
{
  node_hovered = node_id;
}
```

You can also check to see if any node has been selected. Nodes can be clicked on, or they can be selected by clicking and dragging the box selector over them.

```cpp
// Note that since many nodes can be selected at once, we first need to query the number of
// selected nodes before getting them.
const int num_selected_nodes = imnodes::NumSelectedNodes();
if (num_selected_nodes > 0)
{
  std::vector<int> selected_nodes;
  selected_nodes.resize(num_selected_nodes);
  imnodes::GetSelectedNodes(selected_nodes.data());
}
```

See `imnodes.h` for more UI event-related functions.

Like `dear imgui`, the style of the UI can be changed. You can set the color style of individual nodes, pins, and links mid-frame by calling `imnodes::PushColorStyle` and `imnodes::PopColorStyle`.

```cpp
// set the titlebar color of an individual node
imnodes::PushColorStyle(
  imnodes::ColorStyle_TitleBar, IM_COL32(11, 109, 191, 255));
imnodes::PushColorStyle(
  imnodes::ColorStyle_TitleBarSelected, IM_COL32(81, 148, 204, 255));

imnodes::BeginNode(hardcoded_node_id);
// node internals here...
imnodes::EndNode();

imnodes::PopColorStyle();
imnodes::PopColorStyle();
```

If the style is not being set mid-frame, `imnodes::GetStyle` can be called instead, and the values can be set into the style array directly.

```cpp
// set the titlebar color for all nodes
imnodes::Style& style = imnodes::GetStyle();
style.colors[imnodes::ColorStyle_TitleBar] = IM_COL32(232, 27, 86, 255);
style.colors[imnodes::ColorStyle_TitleBarSelected] = IM_COL32(241, 108, 146, 255);
```

## Known issues

* `ImGui::Separator()` spans the current window span. As a result, using a separator inside a node will result in the separator spilling out of the node into the node editor grid.

## Further information

See the `examples/` directory to see library usage in greater detail.

* simple.cpp is a simple hello-world style program which displays two nodes
* save_load.cpp is enables you to add and remove nodes and links, and serializes/deserializes them, so that the program state is retained between restarting the program
* color_node_editor.cpp is a more complete example, which shows how a simple node editor is implemented with a graph.
