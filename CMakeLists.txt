cmake_minimum_required(VERSION 3.15)

project(imnodes)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(imgui CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)


# Imnodes

add_library(imnodes)
target_sources(imnodes PRIVATE
    imnodes.h
    imnodes_internal.h
    imnodes.cpp)
target_include_directories(imnodes PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(imnodes PUBLIC imgui::imgui)


# Example projects

add_executable(colornode
    ${CMAKE_SOURCE_DIR}/imnodes.cpp
    ${CMAKE_SOURCE_DIR}/example/main.cpp
    ${CMAKE_SOURCE_DIR}/example/color_node_editor.cpp)
target_link_libraries(colornode imnodes SDL2::SDL2)
if (APPLE)
    target_link_libraries(colornode "-framework OpenGL")
endif()

add_executable(multieditor
    ${CMAKE_SOURCE_DIR}/imnodes.cpp 
    ${CMAKE_SOURCE_DIR}/example/main.cpp
    ${CMAKE_SOURCE_DIR}/example/multi_editor.cpp)
target_link_libraries(multieditor imnodes SDL2::SDL2)
if (APPLE)
    target_link_libraries(multieditor "-framework OpenGL")
endif()

add_executable(saveload
    ${CMAKE_SOURCE_DIR}/imnodes.cpp
    ${CMAKE_SOURCE_DIR}/example/main.cpp
    ${CMAKE_SOURCE_DIR}/example/save_load.cpp)
target_link_libraries(saveload imnodes SDL2::SDL2)
if (APPLE)
    target_link_libraries(saveload "-framework OpenGL")
endif()

add_executable(hello
    ${CMAKE_SOURCE_DIR}/imnodes.cpp
    ${CMAKE_SOURCE_DIR}/example/main.cpp
    ${CMAKE_SOURCE_DIR}/example/hello.cpp)
target_link_libraries(hello imnodes SDL2::SDL2)
if (APPLE)
    target_link_libraries(hello "-framework OpenGL")
endif()
