# ImGui Node Graph Editor
An ImGui-based framework for creating custom node graph editors, supporting use cases such as dialogue systems, quest systems, and behaviour trees.

<img width="2784" height="1720" alt="example-app" src="https://github.com/user-attachments/assets/abfccb34-b933-4d5d-8f93-7ce665d0c427" />

*The screenshot is from the example app.*

## Table of Contents
 - [Features](https://github.com/georgiitonchev/nodes_graph?tab=readme-ov-file#features)
 - [Dependencies](https://github.com/georgiitonchev/nodes_graph?tab=readme-ov-file#dependencies)
 - [Examples](https://github.com/georgiitonchev/nodes_graph?tab=readme-ov-file#examples)
 - [Usage](https://github.com/georgiitonchev/nodes_graph?tab=readme-ov-file#usage)
 - [Building / Installing](https://github.com/georgiitonchev/nodes_graph?tab=readme-ov-file#building--installing)
 - [Disclaimers, Issues, and Limitations](https://github.com/georgiitonchev/nodes_graph?tab=readme-ov-file#disclaimers-issues-and-limitations)

## Features
  - **Nodes**: Supports basic and group nodes.
  - **Slots**: Slots can act as inputs, outputs, or both.
  - **Connections**: Connections may store extra data, such as type (color) or a value.
  - **Serialization/Deserialization**: Graphs are saved and loaded in JSON format.
  - **Undo/Redo**: Command-pattern–based undo/redo system.
  - **Copy/Paste**: Nodes can be duplicated with customizable clone implementations.
  - **Context Menus**: Extendable context menu options for nodes.
  - **Validation**: Nodes can define and enforce their own validation rules.
  - **Canvas**: Interactive canvas with scrolling and zooming support.
  - **Scaling**: Basic support for different resolutions and DPI scales.

## Dependencies 
  - [ocornut/ImGui](https://github.com/ocornut/imgui): Dear ImGui is a bloat-free graphical user interface library for C++.
  - [nlohmann/json](https://github.com/nlohmann/json): JSON for Modern C++.

### Dependencies for the example app
  - [SDL3](https://github.com/libsdl-org/SDL): Simple DirectMedia Layer (SDL for short) is a cross-platform library designed to make it easy to write multi-media software, such as games and emulators.

## Examples
An example, work in progress, SDL3 app for Windows and macOS can be seen in the [examples](examples/app_sdl3) folder.

## Usage
### Nodes
Nodes are created by implementing the base `Node` class. Each node defines its own `Initialization`, `Drawing`, `Serialization`, `Deserialization`, `Validation`, and `Cloning` implementations.

Here's the full implementation of the [Speech](examples/app_sdl3/src/nodes/speech_node.h) node from the example app:
``` cpp
// speech_node.h

#pragma once
#include "node.h"
#include "../input.h"

class SpeechNode : public Node
{
private:
  // Fields holding the node's data
  std::string _target;
  std::string _text;

  // Initialization
  void _Init() override
  {
    // Optional custom outline color of the node
    _colorOutline = IM_COL32(55, 149, 189, 255);
  
    // All four slots are defined as both input and output by the last two arguments.
    AddSlot(SlotPosition::Left, true, true);
    AddSlot(SlotPosition::Top, true, true);
    AddSlot(SlotPosition::Right, true, true);
    AddSlot(SlotPosition::Bottom, true, true);
  }

  // Drawing
  void _Draw(ImDrawList* drawList) override
  {
    // Input::Text is a simple wrapper around ImGui::InputText, handling dpi scaling and undo/redo functionalities.
    Input::Text("Target", &_target);
    Input::Text("Text", &_text);
  }

  // Serialization
  void _ToJson(nlohmann::json& j) override
  {
    j["target"] = _target;
    j["text"] = _text;
  }

  // Deserialization
  void _FromJson(const nlohmann::json& j) override
  {
    j.at("target").get_to(_target);
    j.at("text").get_to(_text);
  }

  // Validation
  bool _Validate() override
  {
    if (_target.empty())
    {
      SetValidationMessage("Target cannot be empty.");
      return false;
    }
  
    if (_text.empty())
    {
      SetValidationMessage("Text cannot be empty.");
      return false;
    }
  
    return true;
  }

  // Cloning
  Node* _Clone() override
  {
    auto node = new SpeechNode();
    node->_target = _target;
    node->_text = _text;
    return node;
  }
};
```
For nodes to appear as a create option when right-clicking the canvas, they need to be registered at app startup with `NodesGraph::RegisterNode` as seen in the example app [here](https://github.com/georgiitonchev/nodes_graph/blob/03949395b1d8a238aa00b964bc88c03f28374f6c/examples/app_sdl3/src/main.cpp#L650):
``` cpp
// main.cpp

static void RegisterNodes()
{
  // Register the node with the "Speech" label.
  // This will show up in the nodes creation context menu.
  NodesGraph::RegisterNode<SpeechNode>("Speech");
}
```

### Group Nodes
Group nodes are created by implementing the `GroupNode` class. Group nodes must specify the type of child node.

Here’s the full implementation of the [Responses](examples/app_sdl3/src/nodes/responses_node.h) group node (with its child ResponseNode) from the example app.
```cpp
// responses_node.h

#pragma once
#include "node.h"
#include "../input.h"

// Define the child node
class ResponseNode : public Node {
private:
  std::string _text;
  
  void _Init() override
  {
    AddSlot(SlotPosition::Left, false, true);
    AddSlot(SlotPosition::Right, false, true);
  }
  
  void _Draw(ImDrawList* drawList) override
  {
    Input::Text("Text", &_text);
  }
  
  void _ToJson(nlohmann::json& j) override
  {
    j["text"] = _text;
  }
  
  void _FromJson(const nlohmann::json& j) override
  {
    j.at("text").get_to(_text);
  }
  
  bool _Validate() override
  {
    if (_text.empty())
    {
      SetValidationMessage("Text cannot be empty.");
      return false;
    }
    return true;
  }
  
  Node* _Clone() override
  {
    auto clone = new ResponseNode();
    clone->_text = _text;
    return clone;
  }
};

// Define the group node
class ResponsesNode : public GroupNode<ResponseNode> {
private:
  void _Init() override
  {
    _colorOutline = IM_COL32(255, 157, 35, 255);
    AddSlot(SlotPosition::Left, true, false);
    AddSlot(SlotPosition::Top, true, false);
    AddSlot(SlotPosition::Right, true, false);
    AddSlot(SlotPosition::Bottom, true, false);
  }
  
  void _Draw(ImDrawList* drawList) override
  {
  }
  
  void _ToJson(nlohmann::json& j) override
  {
  }
  
  void _FromJson(const nlohmann::json& j) override
  {
  }
  
  Node* _Clone() override
  {
    return new ResponsesNode();
  }
};
```

For a more complete usage example, see the [example](examples/app_sdl3) project.

## Building / Installing
The core framework source files are in the `/src` folder and can be easily copied into your project.

The example project uses CMake and can be built on both Windows and macOS.

```shell
# Clone the repository with its submodules
git clone --recurse-submodules https://github.com/georgiitonchev/nodes_graph.git

# Navigate to the example project
cd nodes_graph/examples/app_sdl3

# Create a folder to contain build files
mkdir build
cd build

# Configure the project
cmake ..

# Build the project
cmake --build .

# Run the app
./Debug/app_sdl3
```
## Disclaimers, Issues, and Limitations
  - This is just a personal project and a work in progress, not a finished or commercial tool, so it’s not perfect and may contain bugs.
  - Due to how zooming is implemented, some ImGui widgets will not render properly.
    - `ImGui::InputTextMultiline`
  - Support for ImGui style colors not yet implemented. `ImGui::StyleColorsDark`, `ImGui::StyleColorsClassic`, `ImGui::StyleColorsLight`
