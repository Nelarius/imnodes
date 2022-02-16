cmake_minimum_required(VERSION 3.16)

project(example)
set(CMAKE_CXX_STANDARD 20)


aux_source_directory(. example_src)

add_executable(color_node_editor main.cpp color_node_editor.cpp)

target_link_directories(color_node_editor PUBLIC
    ${IMGUI} 
    ${IMNODES} 
    ${DEPENDENCIES_DIR}/gl3w/
    ${SDL_DIR}/lib
)

set(LIBS gl3w imgui imnodes SDL2 ${COREFOUNDATION_LIBRARY})

target_link_libraries(color_node_editor ${LIBS})


add_executable(multi_editor main.cpp multi_editor.cpp)

target_link_directories(multi_editor PUBLIC
    ${IMGUI} 
    ${IMNODES} 
    ${DEPENDENCIES_DIR}/gl3w/
    ${SDL_DIR}/lib
)

set(LIBS gl3w imgui imnodes SDL2 ${COREFOUNDATION_LIBRARY})

target_link_libraries(multi_editor ${LIBS})


add_executable(hello main.cpp hello.cpp)

target_link_directories(hello PUBLIC
    ${IMGUI} 
    ${IMNODES} 
    ${DEPENDENCIES_DIR}/gl3w/
    ${SDL_DIR}/lib
)

set(LIBS gl3w imgui imnodes SDL2 ${COREFOUNDATION_LIBRARY})

target_link_libraries(hello ${LIBS})