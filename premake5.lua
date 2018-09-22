workspace "imnodes"
    local project_location = ""
    if _ACTION then
        project_location = "build/" .. _ACTION
    end

    configurations { "Debug", "Release" }

    architecture "x86_64"

    filter "configurations:Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter "action:vs*"
        defines { "_CRT_SECURE_NO_WARNINGS" }

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
        filter "action:vs*"
            -- includedirs { "extern/SDL/include" }

    project "example"
        location(project_location)
        kind "WindowedApp"
        language "C++"
        targetdir "bin/%{cfg.buildcfg}"
        files {"example/main.cpp" }
        includedirs { ".", "example" }
        links { "gl3w", "imgui" }
        filter "system:macosx"
            includedirs { "/Library/Frameworks/SDL2.framework/Headers" }
            links { "Cocoa.framework" }
            linkoptions { "-F/Library/Frameworks -framework SDL2" }