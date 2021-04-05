deps = [imgui_dep, imnodes_dep, gl3w_dep, sdl2]
link_args = []

if target_machine.system() == 'darwin'
    link_args +=  ['-Wl,-framework,CoreFoundation']
endif

executable('hello',
    sources: ['main.cpp', 'hello.cpp'],
    dependencies: deps,
    link_args: link_args,
)

executable('saveload',
    sources: ['main.cpp', 'save_load.cpp'],
    dependencies: deps,
    link_args: link_args,
)

executable('colornode',
    sources: ['main.cpp', 'color_node_editor.cpp'],
    dependencies: deps,
    link_args: link_args,
)

executable('multieditor',
    sources: ['main.cpp', 'multi_editor.cpp'],
    dependencies: deps,
    link_args: link_args,
)
