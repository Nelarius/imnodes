cmake_minimum_required(VERSION 3.11)

project(imnodes)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(external)

if(MSVC)
    add_compile_definitions(SDL_MAIN_HANDLED)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    add_compile_definitions(GL_SILENCE_DEPRECATION)
endif()

# Imnodes
add_library(imnodes)
target_sources(imnodes PRIVATE
    imnodes.h
    imnodes_internal.h
    imnodes.cpp)
target_include_directories(imnodes PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(imnodes PUBLIC imgui)

# Example projects
add_executable(colornode
    ${CMAKE_SOURCE_DIR}/imnodes.cpp
    ${CMAKE_SOURCE_DIR}/example/main.cpp
    ${CMAKE_SOURCE_DIR}/example/color_node_editor.cpp)
target_link_libraries(colornode imnodes imgui-backend)

if(APPLE)
    target_link_libraries(colornode "-framework OpenGL")
elseif(MSVC)
    target_link_libraries(colornode "opengl32")
else()
    target_link_libraries(colornode X11 Xext GL)
endif()

add_executable(multieditor
    ${CMAKE_SOURCE_DIR}/imnodes.cpp
    ${CMAKE_SOURCE_DIR}/example/main.cpp
    ${CMAKE_SOURCE_DIR}/example/multi_editor.cpp)
target_link_libraries(multieditor imnodes imgui-backend)

if(APPLE)
    target_link_libraries(multieditor "-framework OpenGL")
elseif(MSVC)
    target_link_libraries(multieditor "opengl32")
else()
    target_link_libraries(multieditor X11 Xext GL)
endif()

add_executable(saveload
    ${CMAKE_SOURCE_DIR}/imnodes.cpp
    ${CMAKE_SOURCE_DIR}/example/main.cpp
    ${CMAKE_SOURCE_DIR}/example/save_load.cpp)
target_link_libraries(saveload imnodes imgui-backend)

if(APPLE)
    target_link_libraries(saveload "-framework OpenGL")
elseif(MSVC)
    target_link_libraries(saveload "opengl32")
else()
    target_link_libraries(saveload X11 Xext GL)
endif()

add_executable(hello
    ${CMAKE_SOURCE_DIR}/imnodes.cpp
    ${CMAKE_SOURCE_DIR}/example/main.cpp
    ${CMAKE_SOURCE_DIR}/example/hello.cpp)
target_link_libraries(hello imnodes imgui-backend)

if(APPLE)
    target_link_libraries(hello "-framework OpenGL")
elseif(MSVC)
    target_link_libraries(hello "opengl32")
else()
    target_link_libraries(hello X11 Xext GL)
endif()
