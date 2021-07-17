---
name: Bug report
about: Create a report to help us improve
title: ''
labels: ''
assignees: ''

---

1. PLEASE CAREFULLY READ: [Wiki](https://github.com/Nelarius/imnodes/wiki)

2. PLEASE MAKE SURE that you have: read the comments in [imnodes.h](https://github.com/Nelarius/imnodes/blob/master/imnodes.h); explored the [example](https://github.com/Nelarius/imnodes/tree/master/example); searched among Issues; and read the link provided in (1).

3. Be mindful that messages are being sent to the e-mail box of "Watching" users. Try to proof-read your messages before sending them. Edits are not seen by those users.

4. Delete points 1-4 and PLEASE FILL THE TEMPLATE BELOW before submitting your issue or we will immediately close it.

Thank you!

----

## Version of imnodes

Version (or commit): XXX
Operating System: XXX (Windows 10, macOs xx, Ubuntu 20.04, etc.)
Graphics API: XXX (Directx11, Metal, OpenGL 3, etc.)

## My Issue/Question

A clear and concise description of what the issue/question is. Please provide as much context as possible.

## To Reproduce

Steps to reproduce the behavior:
1. Go to '...'
2. Click on '....'
3. Scroll down to '....'
4. See error

## Expected behavior

A clear and concise description of what you expected to happen.

## Screenshots/Video

XXX _(you can drag files here)_

## Standalone, minimal, complete and verifiable example

```cpp
// Here's some code anyone can copy and paste to reproduce your issue
ImGui::Begin("Testing");

    imnodes::BeginNodeEditor();

    imnodes::BeginNode(53);
    ImGui::Dummy(ImVec2(80.0f, 45.0f));
    imnodes::EndNode();

    imnodes::EndNodeEditor();


ImGui::End();
```
