cmake_minimum_required(VERSION 3.16)

project(dependencies)
set(CMAKE_CXX_STANDARD 20)
add_subdirectory(gl3w)

add_subdirectory(imgui)

add_subdirectory(fmt)

add_subdirectory(nlohmann_json)