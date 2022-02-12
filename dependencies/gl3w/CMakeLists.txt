cmake_minimum_required(VERSION 3.16)

set(target gl3w)
project(${target})

set(CMAKE_CXX_STANDARD 17)

add_library(${target} src/gl3w.c)

target_include_directories(${target} PRIVATE include)

