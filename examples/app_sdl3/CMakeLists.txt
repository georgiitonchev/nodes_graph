cmake_minimum_required(VERSION 3.12)

project(app_sdl3)

if (CMAKE_CONFIGURATION_TYPES)
    set (CONFIG_DIR "$<CONFIG>")
elseif (CMAKE_BUILD_TYPE)
    set (CONFIG_DIR "${CMAKE_BUILD_TYPE}")
else()
    set (CONFIG_DIR "Debug")
endif() 

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CONFIG_DIR}")

add_subdirectory(ext/SDL EXCLUDE_FROM_ALL)

add_executable(app_sdl3)

target_sources(app_sdl3 PRIVATE
    # Main
    src/main.cpp
    src/input.h

    # ImGui
    ../../ext/imgui/imgui.cpp
    ../../ext/imgui/imgui_demo.cpp
    ../../ext/imgui/imgui_draw.cpp
    ../../ext/imgui/imgui_tables.cpp
    ../../ext/imgui/imgui_widgets.cpp
    ../../ext/imgui/imgui_stdlib.cpp
    ../../ext/imgui/imgui_internal.h
    ../../ext/imgui/imgui_stdlib.h
    ../../ext/imgui/imgui_impl_sdl3.cpp
    ../../ext/imgui/imgui_impl_sdl3.h
    ../../ext/imgui/imgui_impl_sdlrenderer3.cpp
    ../../ext/imgui/imgui_impl_sdlrenderer3.h

    # JSON
    ../../ext/json/json.h

    # Core
    ../../src/literals.h
    ../../src/nodes_graph_settings.h
    ../../src/nodes_graph.h
    ../../src/nodes_graph.cpp
    ../../src/node.h
    ../../src/node.cpp
    ../../src/node_slot.h
    ../../src/node_slot.cpp
    ../../src/node_connection.h
    ../../src/node_connection.cpp

    # Nodes
    src/nodes/speech_node.h
    src/nodes/responses_node.h
    src/nodes/entry_node.h
    src/nodes/connector_in_node.h
    src/nodes/connector_out_node.h
    src/nodes/exit_node.h
)

set_property(TARGET app_sdl3 PROPERTY CXX_STANDARD 20)

target_link_libraries(app_sdl3 PRIVATE SDL3::SDL3)

target_include_directories(app_sdl3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/json)