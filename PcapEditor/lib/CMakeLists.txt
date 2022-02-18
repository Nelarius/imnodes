cmake_minimum_required(VERSION 3.16)

set(target libgraph)
project(${target})

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

include_directories(include ${PcapPlusPlus_DIR}/include)
if(APPLE)
    include_directories( ${MACOS_SDK_HOME}/usr/include/netinet)
endif()

link_directories(${PcapPlusPlus_DIR}/lib)

aux_source_directory(src graph_src)
add_library(${target} ${graph_src})

link_directories(
    ${PcapPlusPlus_DIR}/lib

)

IF (UNIX)
	MESSAGE(STATUS "Now is UNIX-like OS's.")
    set(unix_libs dl)
ENDIF ()
IF(APPLE)
    target_link_libraries(${target} ${SYSTEMCONFIGURATION_LIBRARY} ${COREFOUNDATION_LIBRARY})
ENDIF()

target_link_libraries(${target}   -lpthread  ${unix_libs} Common++ Packet++ Pcap++ -lpcap)

set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
