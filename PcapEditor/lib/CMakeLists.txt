cmake_minimum_required(VERSION 3.16)

set(target libgraph)
project(${target})

set(CMAKE_CXX_STANDARD 20)

include_directories(include ${PcapPlusPlus_DIR}/include ${MACOS_SDK_HOME}/usr/include/netinet)
link_directories(${PcapPlusPlus_DIR}/lib)

aux_source_directory(src graph_src)
add_library(${target} ${graph_src})

target_link_directories(${target} PUBLIC
    ${PcapPlusPlus_DIR}/lib

)
target_link_libraries(${target} Common++ Packet++ Pcap++ -lpcap -lpthread ${SYSTEMCONFIGURATION_LIBRARY} ${COREFOUNDATION_LIBRARY})
