cmake_minimum_required(VERSION 3.20)
# set(CMAKE_OSX_DEPLOYMENT_TARGET "12.3" CACHE STRING "Minimum Mac OS X deployment version" FORCE)

project(imnodes)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(imgui CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

if(MSVC)
    add_compile_definitions(SDL_MAIN_HANDLED)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Imnodes

add_library(imnodes)
target_sources(imnodes PRIVATE
    imnodes.h
    imnodes_internal.h
    imnodes.cpp)
target_include_directories(imnodes PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(imnodes PUBLIC imgui::imgui)

# Python

message("*****************************************")
set(CMAKE_FIND_FRAMEWORK LAST)
set(Python_FIND_VIRTUALENV FIRST)
set(Python_FIND_UNVERSIONED_NAMES FIRST)
message("MAC OS " ${CMAKE_OSX_SYSROOT})
message("Python_ROOT " ${Python_ROOT})
find_package(Python REQUIRED COMPONENTS Interpreter Development)

message("Python_EXECUTABLE " ${Python_EXECUTABLE})
message("Python_FOUND " ${Python_FOUND})
message("Python_Interpreter_FOUND " ${Python_Interpreter_FOUND})
message("Python_Development_FOUND " ${Python_Development_FOUND})
   
message("Python_LIBRARIES " ${Python_LIBRARIES})
message("Python_LIBRARY_DIRS " ${Python_LIBRARY_DIRS})
message("Python_INCLUDE_DIRS " ${Python_INCLUDE_DIRS})
message("Python_LINK_OPTIONS " ${Python_LINK_OPTIONS})
message("Python_EXECUTABLE " ${Python_EXECUTABLE})
message("Python_INTERPRETER_ID " ${Python_INTERPRETER_ID})
  
message("Python_VERSION " ${Python_VERSION})
message("Python_VERSION_MAJOR " ${Python_VERSION_MAJOR})
message("Python_VERSION_MINOR " ${Python_VERSION_MINOR})
message("*****************************************")

# Example projects

add_executable(colornode
    ${CMAKE_SOURCE_DIR}/imnodes.cpp
    ${CMAKE_SOURCE_DIR}/example/main.cpp
    ${CMAKE_SOURCE_DIR}/example/color_node_editor.cpp)
target_link_libraries(colornode imnodes SDL2::SDL2)
if (APPLE)
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
target_link_libraries(multieditor imnodes SDL2::SDL2)
if (APPLE)
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
target_link_libraries(saveload imnodes SDL2::SDL2)
if (APPLE)
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
target_link_libraries(hello imnodes SDL2::SDL2)
if (APPLE)
    target_link_libraries(hello "-framework OpenGL")
elseif(MSVC)
    target_link_libraries(hello "opengl32")
else()
    target_link_libraries(hello X11 Xext GL)
endif()


add_executable(Dynamite
    ${CMAKE_SOURCE_DIR}/imnodes.cpp
    ${CMAKE_SOURCE_DIR}/example/Dynamite/main.cpp
    ${CMAKE_SOURCE_DIR}/example/Dynamite/dynamite.cpp
    ${CMAKE_SOURCE_DIR}/example/Dynamite/ui.cpp
    ${CMAKE_SOURCE_DIR}/example/Dynamite/menubar.cpp
    ${CMAKE_SOURCE_DIR}/example/Dynamite/block.cpp
    ${CMAKE_SOURCE_DIR}/example/Dynamite/context.cpp
    ${CMAKE_SOURCE_DIR}/example/Dynamite/editor.cpp
    ${CMAKE_SOURCE_DIR}/example/Dynamite/palette.cpp
    ${CMAKE_SOURCE_DIR}/example/Dynamite/multipanel.cpp
    ${CMAKE_SOURCE_DIR}/example/Dynamite/dyndsp_wrapper.cpp
    ${CMAKE_SOURCE_DIR}/example/Dynamite/JsonGraphFileWriter.cpp
    )
target_link_libraries(Dynamite imnodes SDL2::SDL2)
target_link_libraries(Dynamite ${Python_LIBRARIES})
target_include_directories(Dynamite PRIVATE ${Python_INCLUDE_DIRS})
if (APPLE)
    target_link_libraries(Dynamite "-framework OpenGL")
elseif(MSVC)
    target_link_libraries(Dynamite "opengl32")
else()
    target_link_libraries(Dynamite X11 Xext GL)
endif()