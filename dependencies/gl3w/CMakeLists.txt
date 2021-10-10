cmake_minimum_required(VERSION 3.16)

set(target gl3w)
project(${target})

set(CMAKE_CXX_STANDARD 17)

# set_source_files_properties(src/gl3w.c PROPERTIES LANGUAGE CXX )

add_library(${target} src/gl3w.c)

target_include_directories(${target} PUBLIC include)

