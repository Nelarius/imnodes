newoption {
    trigger     = "sdl-include-path",
    value       = "path",
    description = "The location of your SDL2 header files"
}

newoption {
    trigger     = "sdl-link-path",
    value       = "path",
    description = "The location of your SDL2 link libraries"
}

newoption {
    trigger     = "use-sdl-framework",
    description = "Use the installed SDL2 framework (on MacOS)"
}

local projectlocation = os.getcwd()
local gl3wlocation = path.join(os.getcwd(), "dependencies/gl3w")
local imguilocation = path.join(os.getcwd(), "dependencies/imgui-1.84.2")

if _ACTION then
    projectlocation = path.join(projectlocation, "build", _ACTION)
end

function imnodes_example_project(name, example_file)
    project(name)
    location(projectlocation)
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++11"
    targetdir "bin/%{cfg.buildcfg}"
    debugdir "bin/%{cfg.buildcfg}"
    files {"example/main.cpp", path.join("example", example_file) }
    includedirs {
        os.getcwd(),
        imguilocation,
        path.join(gl3wlocation, "include"),
    }
    links { "gl3w", "imgui", "imnodes" }

    defines { "IMGUI_IMPL_OPENGL_LOADER_GL3W" }

    if _OPTIONS["sdl-include-path"] then
        includedirs { _OPTIONS["sdl-include-path"] }
    end

    if _OPTIONS["sdl-link-path"] then
        libdirs { _OPTIONS["sdl-link-path"] }

        filter "system:macosx"
            links {
                "iconv",
                "AudioToolbox.framework",
                "Carbon.framework",
                "Cocoa.framework",
                "CoreAudio.framework",
                "CoreVideo.framework",
                "ForceFeedback.framework",
                "IOKit.framework"
            }
        filter "*"
    end

    if _OPTIONS["use-sdl-framework"] then
        includedirs { "/Library/Frameworks/SDL2.framework/Headers" }
        linkoptions { "-F/Library/Frameworks -framework SDL2 -framework CoreFoundation" }
    else
        links { "SDL2" }
    end

    filter "system:windows"
        defines { "SDL_MAIN_HANDLED" }
        links { "opengl32" }
        if _OPTIONS["sdl-link-path"] then
            postbuildcommands { 
                "{COPY} " .. 
                path.join(os.getcwd(), _OPTIONS["sdl-link-path"].."/../bin/", "SDL2.dll") .. 
                " %{cfg.targetdir}" }
        end

    filter "system:linux"
        links { "dl" }
end

workspace "imnodes"
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

    startproject "colornode"

    group "dependencies"

    project "gl3w"
        location(projectlocation)
        kind "StaticLib"
        language "C"
        targetdir "lib/%{cfg.buildcfg}"
        files { path.join(gl3wlocation, "src/gl3w.c") }
        includedirs { path.join(gl3wlocation, "include") }

    project "imgui"
        location(projectlocation)
        kind "StaticLib"
        language "C++"
        cppdialect "C++98"
        targetdir "lib/%{cfg.buildcfg}"
        files { path.join(imguilocation, "**.cpp") }
        includedirs {
            imguilocation,
            path.join(gl3wlocation, "include") }

        if _OPTIONS["sdl-include-path"] then
            includedirs { _OPTIONS["sdl-include-path"] }
        end

        if _OPTIONS["use-sdl-framework"] then
            includedirs { "/Library/Frameworks/SDL2.framework/Headers" }
        end

    group "imnodes"

    project "imnodes"
        location(projectlocation)
        kind "StaticLib"
        language "C++"
        cppdialect "C++98"
        enablewarnings { "all" }
        targetdir "lib/%{cfg.buildcfg}"
        files { "imnodes.h", "imnodes_internal.h", "imnodes.cpp" }
        includedirs { path.join(imguilocation) }

    group "examples"

    imnodes_example_project("hello", "hello.cpp")

    imnodes_example_project("saveload", "save_load.cpp")

    imnodes_example_project("colornode", "color_node_editor.cpp")

    imnodes_example_project("multieditor", "multi_editor.cpp")
