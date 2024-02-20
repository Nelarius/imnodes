include(FetchContent)

# Declare
FetchContent_Declare(imgui
    GIT_REPOSITORY "https://github.com/ocornut/imgui"
    GIT_TAG b81bd7ed984ce095c20a059dd0f4d527e006998f # v1.90.0
    GIT_SHALLOW ON)

FetchContent_Declare(glfw
    GIT_REPOSITORY "https://github.com/glfw/glfw"
    GIT_TAG 7482de6 # 3.3.8
    GIT_SHALLOW ON)

# Fetch
message(STATUS "Fetcing imgui...")
FetchContent_MakeAvailable(imgui)
set(IMGUI_SOURCE_DIR ${imgui_SOURCE_DIR})

message(STATUS "Fetching glfw...")
FetchContent_MakeAvailable(glfw)

# Configure

# imgui
set(IMGUI_INCLUDE_DIR ${IMGUI_SOURCE_DIR} CACHE INTERNAL "imgui Include Directory")
add_library(imgui STATIC
    ${IMGUI_SOURCE_DIR}/imgui.cpp
    ${IMGUI_SOURCE_DIR}/imgui_demo.cpp
    ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
    ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp
    ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_INCLUDE_DIR})

add_library(imgui-backend STATIC
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)
target_include_directories(imgui-backend PUBLIC ${IMGUI_SOURCE_DIR}/backends)
target_link_libraries(imgui-backend PUBLIC imgui glfw)
