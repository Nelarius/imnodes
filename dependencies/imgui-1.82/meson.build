# for real build definition see https://github.com/mesonbuild/imgui/tree/1.81
include_dirs = include_directories('.')
sources = ['imgui.cpp', 'imgui_draw.cpp', 'imgui_impl_opengl3.cpp', 'imgui_impl_sdl.cpp', 'imgui_tables.cpp', 'imgui_widgets.cpp']

imgui = static_library('imgui', sources,
    cpp_args : '-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS',
    dependencies: [gl3w_dep, sdl2],
    include_directories: include_dirs,
)

imgui_dep = declare_dependency(include_directories: include_dirs, link_with: imgui)
