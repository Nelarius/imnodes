cmake_minimum_required(VERSION 3.16)

set(target imnodes)
project(${target})
set(CMAKE_CXX_STANDARD 20)
include_directories(${IMGUI})

aux_source_directory(. imnodes_src)

add_library(${target} ${imnodes_src})
