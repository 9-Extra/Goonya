add_rules("mode.debug", "mode.release")

set_languages("c23", "c++23")
set_warnings("all")

set_encodings("utf-8")

add_requires("nowide_standalone")
add_requires("imgui", {configs = {glfw_opengl3 = true}})
add_requires("glfw")
add_requires("freeimage", {configs = {shared = true}})
-- 在Windows上，spdlog需要SPDLOG_UTF8_TO_WCHAR_CONSOLE以将输出到命令行的日志转换为wchar以避免乱码，此设置还需要开启wchar支持
add_requires("spdlog", {configs = {wchar = true, std_format = true, header_only = false, cxxflags = {"-DSPDLOG_UTF8_TO_WCHAR_CONSOLE"}}})
add_requires("jsoncpp")

if is_mode("debug") then
    add_defines("DEBUG")
else 
    add_defines("NDEBUG")
end

target("glad")
    set_kind("object")
    add_files("thirdparty/glad/src/glad.c")
    add_includedirs("thirdparty/glad/include", {public = true, private=true})

target("GRuntime")
    set_kind("static")
    add_files("sources/runtime/src/**.cpp")
    add_includedirs("sources/runtime/src", {public = true, private=true})
    add_deps("glad")
    add_defines("GLFW_INCLUDE_NONE") -- 手动包含glad

    add_packages("glfw")
    add_packages("imgui", "spdlog", "jsoncpp", "nowide_standalone", "freeimage", {public=true})

    add_includedirs("sources/runtime/include", {public = true})

target("GDemo")
    set_kind("binary")
    add_files("sources/game/src/**.cpp")
    add_deps("GRuntime")
    set_rundir("bin")
