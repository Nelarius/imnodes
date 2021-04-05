include_dirs = include_directories('include')

gl3w = static_library('gl3w',
    sources: 'src/gl3w.c',
    include_directories: include_dirs,
)

gl3w_dep = declare_dependency(include_directories: include_dirs, link_with: gl3w)
