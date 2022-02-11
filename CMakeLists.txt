cmake_minimum_required(VERSION 3.16)

project(imnodes)


IF (APPLE)
   FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation )
   message("[+]CoreFoundation found in ${COREFOUNDATION_LIBRARY}")
ENDIF (APPLE)




set(CMAKE_CXX_STANDARD 17)

# doesn't work ? 
# IF (APPLE)
#    FIND_LIBRARY(SDL2_LIBRARY SDL2 )
#    message("[+]SDL2 found in ${SDL2_LIBRARY}")
#    find_package(SDL2)
#    if (SDL2_FOUND)
#     message("[+]SDL2 found in ${SDL2_INCLUDE_DIR}")
#    endif(SDL2_FOUND)
   
# ENDIF (APPLE)

# hard code , you should update it with your sdl2 path
set(SDL2_PATH /usr/local/Cellar/sdl2/2.0.20)
set(SDL_DIR ${SDL2_PATH})

include_directories(${SDL_DIR}/include/SDL2) # 添加SDL头文件搜索路径
link_libraries(${SDL_DIR}/lib/) # 增加SDL链接库目录


set(IMGUI  ${CMAKE_SOURCE_DIR}/dependencies/imgui-1.84.2)

set(IMNODES  ${CMAKE_SOURCE_DIR}/imnodes)

set(GL3W  ${CMAKE_SOURCE_DIR}/dependencies/gl3w/include)

include_directories(
    ${IMGUI} 
    ${IMNODES} 
    ${GL3W}
    )

# set(GL3W_INCLUDE ${CMAKE_SOURCE_DIR}/dependencies/include)

add_subdirectory(dependencies)

add_subdirectory(imnodes)

add_subdirectory(example)

