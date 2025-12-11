target("GRuntime")
    set_kind("static")
    add_files("src/**.cpp")
    add_includedirs("src", {public = true})
    add_deps("glad", "stb")
    add_defines("GLFW_INCLUDE_NONE") -- 手动包含glad
    
    add_packages("glfw")
    add_packages("imgui", "spdlog", "jsoncpp", "reflect-cpp", {public=true})
    if is_plat("linux") then
        add_syslinks("stdc++exp") -- 用于支持stackstrace
    end 
    add_includedirs("include", {public = true})

-- 测试
target("test_math")
    set_kind("binary")
    add_tests("default")
    set_default(false)
    add_files("test/math.cpp")
    add_includedirs("src", "include")
    add_packages("gtest")

target("test_gobject")
    set_kind("binary")
    add_tests("default")
    set_default(false)
    add_files("test/GObject.cpp", "src/function/world/GObject.cpp", "src/core/log/*.cpp")
    add_includedirs("src", "include")
    add_packages("gtest", "spdlog")


