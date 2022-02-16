cmake_minimum_required(VERSION 3.16)

project(imnodes)


IF (APPLE)
   FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation )
   message("[+]CoreFoundation found in ${COREFOUNDATION_LIBRARY}")
ENDIF (APPLE)




set(CMAKE_CXX_STANDARD 20)


if(SDL_PATH)
    #do something
    set(SDL_DIR ${SDL_PATH})
else()
    #do something
    set(SDL_PATH /usr/local/Cellar/sdl2/2.0.20)
    message("[-] SDL2 not found; set ${SDL_PATH} default!")
    set(SDL_DIR ${SDL_PATH})
endif()


include_directories(${SDL_DIR}/include/SDL2) # 添加SDL头文件搜索路径
link_libraries(${SDL_DIR}/lib/) # 增加SDL链接库目录


set(DEPENDENCIES_DIR ${CMAKE_SOURCE_DIR}/dependencies)
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -g -O0")

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

# add_subdirectory(example)

add_subdirectory(PcapEditor)

