add_requires("imgui", {configs = {glfw_opengl3 = true}})
add_requires("glfw", {system = false}) -- 最好用系统自带的
-- 在Windows上，spdlog需要SPDLOG_UTF8_TO_WCHAR_CONSOLE以将输出到命令行的日志转换为wchar以避免乱码，此设置还需要开启wchar支持
-- 在Linux上，可能调用了系统自带的spdlog库，而其编译选项可能不是想要的，所以指定system为false
add_requires("spdlog", {system = false, configs = {wchar = true, std_format = true, header_only = false, cxxflags = {"-DSPDLOG_UTF8_TO_WCHAR_CONSOLE"}}})
add_requires("jsoncpp")
add_requires("reflect-cpp")
add_requires("gtest")

if is_plat("linux") then
    add_syslinks("stdc++exp") -- 用于支持stackstrace
end 

includes("runtime", "game")