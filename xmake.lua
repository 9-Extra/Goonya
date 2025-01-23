add_rules("mode.debug", "mode.release")

set_languages("c23", "c++23")
set_warnings("all")

add_requires("imgui", {configs = {glfw_opengl3 = true}})
add_requires("glfw")
add_requires("freeimage")
add_requires("spdlog")
add_requires("jsoncpp")

if is_mode("debug") then
    add_defines("DEBUG")
else 
    add_defines("NDEBUG")
end

target("glad")
    set_kind("static")
    add_files("thirdparty/glad/src/glad.c")
    add_includedirs("thirdparty/glad/include", {public = true, private=true})

target("GRuntime")
    set_kind("static")
    add_files("sources/runtime/src/**.cpp")
    add_includedirs("sources/runtime/src", {public = true, private=true})
    add_deps("glad")
    add_defines("GLFW_INCLUDE_NONE")

    add_packages("glfw", "freeimage")
    add_packages("imgui", "spdlog", "jsoncpp", {public=true})

    add_includedirs("sources/runtime/include", {public = true})

target("GDemo")
    set_kind("binary")
    add_files("sources/game/src/**.cpp")
    add_deps("GRuntime")
    set_rundir("bin")
