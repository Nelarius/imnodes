cmake_minimum_required(VERSION 3.16)

project(imnodes)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules;${CMAKE_MODULE_PATH}")

include("${CMAKE_SOURCE_DIR}/cmake/modules/detect.cmake")

detectOS()
detectCompiler()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")

IF (APPLE)
   FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation )
   message("[+]CoreFoundation found in ${COREFOUNDATION_LIBRARY}")
   FIND_LIBRARY(SYSTEMCONFIGURATION_LIBRARY SystemConfiguration )
   message("[+]CoreFoundation found in ${SYSTEMCONFIGURATION_LIBRARY}") 
ENDIF (APPLE)




set(CMAKE_CXX_STANDARD 20)


find_package(SDL2)
if(SDL2_FOUND)
    # message("[+] found sdl2 include ${SDL2_INCLUDE_DIR}")
    # message("[+] found sdl2 library ${SDL2_LIBRARY}")
    include_directories(${SDL2_INCLUDE_DIR}) # 添加SDL头文件搜索路径
    link_directories(${SDL2_LIBRARY}) # 增加SDL链接库目录
else()
	message(FATAL_ERROR "[-]sdl2 not found")
endif()


find_package(PcapPlusPlus)
if(PcapPlusPlus_FOUND)
    include_directories( ${PcapPlusPlus_INCLUDE_DIR} )
    link_directories(${PcapPlusPlus_LIBS_DIR}) # 增加SDL链接库目录
    # message("${PcapPlusPlus_LIBS_DIR}")
else()
    message(FATAL_ERROR "[-]pcapplusplus not found")
endif()

set(DEPENDENCIES_DIR ${CMAKE_SOURCE_DIR}/dependencies)
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

set(IMGUI  ${DEPENDENCIES_DIR}/imgui)
set(GL3W  ${DEPENDENCIES_DIR}/gl3w/include)

set(IMNODES  ${CMAKE_SOURCE_DIR}/imnodes)

set(FMT ${DEPENDENCIES_DIR}/fmt/include)

set(JSON ${DEPENDENCIES_DIR}/nlohmann_json/include)

# 包含头文件，增加链接文件夹

include_directories(
    ${IMGUI} 
    ${IMNODES} 
    ${GL3W}
    ${FMT}
    ${JSON}
    )


add_subdirectory(dependencies)

add_subdirectory(imnodes)

add_subdirectory(PcapEditor)

