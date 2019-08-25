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
        targetdir "lib/%{cfg.buildcfg}"
        files { "imgui/**.h", "imgui/**.cpp" }
        includedirs { "imgui", "gl3w/include" }
        filter "system:macosx"
            includedirs { "/Library/Frameworks/SDL2.framework/Headers" }

    project "example"
        location(project_location)
        kind "WindowedApp"
        language "C++"
        targetdir "bin/%{cfg.buildcfg}"
        files {"example/main.cpp", "example/simple.cpp", "imnodes.cpp" }
        includedirs { ".", "imgui", "gl3w/include" }
        links { "gl3w", "imgui" }
        filter { "action:gmake" }
            buildoptions { "-std=c++11" }
        filter "system:macosx"
            includedirs { "/Library/Frameworks/SDL2.framework/Headers" }
            linkoptions { "-F/Library/Frameworks -framework SDL2 -framework CoreFoundation" }

    project "saveload"
        location(project_location)
        kind "WindowedApp"
        language "C++"
        targetdir "bin/%{cfg.buildcfg}"
        files {"example/main.cpp", "example/save_load.cpp", "imnodes.cpp" }
        includedirs { ".", "imgui", "gl3w/include" }
        links { "gl3w", "imgui" }
        filter { "action:gmake" }
            buildoptions { "-std=c++11" }
        filter "system:macosx"
            includedirs { "/Library/Frameworks/SDL2.framework/Headers" }
            linkoptions { "-F/Library/Frameworks -framework SDL2 -framework CoreFoundation" }

    project "colornode"
        location(project_location)
        kind "WindowedApp"
        language "C++"
        targetdir "bin/%{cfg.buildcfg}"
        files {"example/main.cpp", "example/color_node_editor.cpp", "imnodes.cpp" }
        includedirs { ".", "imgui", "gl3w/include" }
        links { "gl3w", "imgui" }
        filter { "action:gmake" }
            buildoptions { "-std=c++11" }
        filter "system:macosx"
            includedirs { "/Library/Frameworks/SDL2.framework/Headers" }
            linkoptions { "-F/Library/Frameworks -framework SDL2 -framework CoreFoundation" }