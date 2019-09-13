<h1 align="center">imnodes</h1>

<p align="center">A small, dependency-free node editor extension for <a href="https://github.com/ocornut/imgui">dear imgui</a>.</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/Nelarius/imnodes/master/img/imnodes.gif?token=ADH_jEpqbBrw0nH-BUmOip490dyO2CnRks5cVZllwA%3D%3D">
</p>

Features:

* The only dependency is `dear imgui` itself.
* `dear imgui`-inspired immediate mode API.
* Single header file, and single source file. Just copy-paste `imnodes.h` and `imnodes.cpp` into your project.
* Written in the same style of C++ as `dear imgui` itself -- no modern C++ used.
* Use regular `dear imgui` widgets within the nodes

## A brief tour

Here is a small overview of how the extension is used. For more information on example usage, scroll to the bottom of the README.

Before anything can be done, the library must be initialized. This can be done at the same time as `dear imgui` initialization.

```cpp
ImGui::Initialize();
imnodes::Initialize();

// elsewhere in the code...
imnodes::Shutdown();
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
imnodes::SetNodeName(hardcoded_node_id, "empty node");

imnodes::BeginNodeEditor();

imnodes::BeginNode(hardcoded_node_id);
imnodes::EndNode();

imnode::EndNodeEditor();
```

Nodes, like windows in `dear imgui` must be uniquely identified. But we can't use the node titles for identification, because it should be possible to have many nodes of the same name in the workspace. Instead, you just use integers for identification.

Attributes are the UI content of the node. An attribute will have a pin (the little circle) on either side of the node. There are two types of attributes: input, and output attributes. Input attribute pins are on the left side of the node, and output attribute pins are on the right. Like nodes, pins must be uniquely identified.

```cpp
imnodes::SetNodeName(hardcoded_node_id, "output node");
imnodes::BeginNode(hardcoded_node_id);

const int output_attr_id = 2;
imnodes::BeginOutputAttribute(output_attr_id);
// in between Begin|EndAttribute calls, you can call ImGui
// UI functions
ImGui::Text("output pin");
ImGui::EndAttribute();

imnodes::EndNode();
```

The extension doesn't really care what is in the attribute. It just renders the pin for the attribute, and allows the user to create links between pins.

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

After `EndNodeEditor` has been called, you can test if a link was created during the frame with the function call `IsLinkCreated`:

```
int start_attr, end_attr;
if (IsLinkCreated(&start_attr, &end_attr))
{
  links.push_back(std::make_pair(start_attr, end_attr));
}
```

In addition to testing for new links, you can also test for whether UI elements are being hovered over or selected (clicked).

```cpp
int node_id;
if (IsNodeSelected(&node_id))
{
  node_selected = node_id;
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

imnodes::SetNodeName(hardcoded_node_id, "colorful node");

imnodes::BeginNode(hardcoded_node_id);
// node internals here...
imnodes::EndNode();

imnodes::PopColorStyle();
imnodes::PopColorStyle();
```

If the style is not being set mid-frame, `imnodes::GetStyle` can be called instead, and the values can be set into the style array directly.

```cpp
// set the titlebar color for all nodes
Style& style = imnodes::GetStyle();
style.colors[imnodes::ColorStyle_TitleBar] = IM_COL32(232, 27, 86, 255);
style.colors[imnodes::ColorStyle_TitleBarSelected] = IM_COL32(241, 108, 146, 255);
```

## Further information

See the `examples/` directory to see library usage in greater detail.

* simple.cpp is a simple hello-world style program which displays two nodes
* save_load.cpp is enables you to add and remove nodes and links, and serializes/deserializes them, so that the program state is retained between restarting the program
* color_node_editor.cpp is a more complete example, which shows how a simple node editor is implemented with a graph.
