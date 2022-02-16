cmake_minimum_required(VERSION 3.16)
set(target pcap_editor)
project(${target})
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(lib)

aux_source_directory(. pcap_editor_src)

add_executable(${target} main.cpp pcap_editor.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/include)

target_link_directories(${target} PUBLIC
    ${IMGUI} 
    ${IMNODES} 
    ${DEPENDENCIES_DIR}/gl3w/
    ${SDL_DIR}/lib
    ${DEPENDENCIES_DIR}/fmt/

    
)

set(LIBS gl3w imgui imnodes SDL2 ${COREFOUNDATION_LIBRARY} libgraph fmt nlohmann_json)

target_link_libraries(${target} ${LIBS})
