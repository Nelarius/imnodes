project('imnodes_proj', 'c', 'cpp',
    default_options : 'cpp_std=c++11',
)

sdl2 = dependency('sdl2')

subdir('dependencies')

imnodes_include = include_directories('.')
imnodes = static_library('imnodes',
    dependencies: imgui_dep,
    sources: 'imnodes.cpp',
    include_directories: imnodes_include,
)
imnodes_dep = declare_dependency(include_directories: imnodes_include, link_with: imnodes)

subdir('example')
