cmake_minimum_required(VERSION 3.16)

project(example)
set(CMAKE_CXX_STANDARD 17)


aux_source_directory(. example_src)

add_executable(main main.cpp color_node_editor.cpp)

target_link_directories(main PUBLIC
    ${IMGUI} 
    ${IMNODES} 
    ${DEPENDENCIES_DIR}/gl3w/
    ${SDL_DIR}/lib
)

set(LIBS gl3w imgui imnodes SDL2 ${COREFOUNDATION_LIBRARY})

target_link_libraries(main ${LIBS})
