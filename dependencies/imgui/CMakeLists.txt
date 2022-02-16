cmake_minimum_required(VERSION 3.16)

set(target imgui)
project(${target})
set(CMAKE_CXX_STANDARD 20)

aux_source_directory(. imgui_src)

add_library(${target} ${imgui_src})
target_include_directories(${target} PUBLIC .)
