function imnodes_example_project(name, example_file)
    project(name)
    location(project_location)
    kind "WindowedApp"
    language "C++"
    targetdir "bin/%{cfg.buildcfg}"
    files {"example/main.cpp", path.join("example", example_file) }
    includedirs { ".", "imgui", "gl3w/include" }
    links { "gl3w", "imgui", "imnodes" }
    filter { "action:gmake" }
        buildoptions { "-std=c++11" }
    filter "system:macosx"
        includedirs { "/Library/Frameworks/SDL2.framework/Headers" }
        linkoptions { "-F/Library/Frameworks -framework SDL2 -framework CoreFoundation" }
    filter "system:linux"
        includedirs { "/usr/include/SDL2" }
        links { "SDL2", "dl" }
end

workspace "imnodes"
    local project_location = ""
    if _ACTION then
        project_location = "build/" .. _ACTION
    end

    configurations { "Debug", "Release" }
    architecture "x86_64"
    defines { "IMGUI_DISABLE_OBSOLETE_FUNCTIONS" }

    filter "configurations:Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter "action:vs*"
        defines { "_CRT_SECURE_NO_WARNINGS" }

    warnings "Extra"

    startproject "example"

    project "gl3w"
        location(project_location)
        kind "StaticLib"
        language "C"
        targetdir "lib/%{cfg.buildcfg}"
        files { "gl3w/src/gl3w.c" }
        includedirs { "gl3w/include" }

    project "imgui"
        location(project_location)
        kind "StaticLib"
        language "C++"
        cppdialect "C++98"
        targetdir "lib/%{cfg.buildcfg}"
        files { "imgui/**.h", "imgui/**.cpp" }
        includedirs { "imgui", "gl3w/include" }
        filter "system:macosx"
            includedirs { "/Library/Frameworks/SDL2.framework/Headers" }
        filter "system:linux"
            -- NOTE: This is to support inclusion via #include <SDL.h>.
            -- Otherwise we would have to do <SDL2/SDL.h> which would not
            -- be compatible with the macOS framework
            includedirs { "/usr/include/SDL2" }

    project "imnodes"
        location(project_location)
        kind "StaticLib"
        language "C++"
        cppdialect "C++98"
        enablewarnings { "all" }
        targetdir "lib/%{cfg.buildcfg}"
        files { "imnodes.h", "imnodes.cpp" }
        includedirs { "imgui" }

    imnodes_example_project("simple", "simple.cpp")

    imnodes_example_project("saveload", "save_load.cpp")

    imnodes_example_project("colornode", "color_node_editor.cpp")

    imnodes_example_project("multieditor", "multi_editor.cpp")
