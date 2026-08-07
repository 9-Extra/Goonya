target("GRuntime")
    set_kind("static")
    add_files("src/**.cpp")
    add_includedirs("src", {public = true})
    add_deps("glad", "stb")
    add_defines("GLFW_INCLUDE_NONE") -- 手动包含glad
    
    add_packages("glfw")
    add_packages("imgui", "spdlog", "jsoncpp", "reflect-cpp", {public=true})
    add_includedirs("include", {public = true})

-- 测试
target("deps_for_test")
    set_kind("static")
    add_files("src/core/log/*.cpp", "src/core/GAssert.cpp")
    add_includedirs("src", "include", {public = true})
    add_packages("gtest", "spdlog", {public = true})

target("test_math")
    set_kind("binary")
    add_tests("default")
    set_default(false)
    add_files("test/math.cpp")
    add_deps("deps_for_test")

target("test_gobject")
    set_kind("binary")
    add_tests("default")
    set_default(false)
    add_files("test/GObject.cpp", "src/function/world/GObject.cpp")
    add_deps("deps_for_test")

target("test_sparse_set")
    set_kind("binary")
    set_warnings("none")
    add_tests("default")
    set_default(false)
    add_files("test/sparse_set.cpp")
    add_deps("deps_for_test")

target("test_future")
    set_kind("binary")
    add_tests("default")
    set_default(false)
    add_files("test/future.cpp", "src/core/ThreadPool.cpp", "src/core/ThreadUtils.cpp")
    add_deps("deps_for_test")

target("test_future_void")
    set_kind("binary")
    add_tests("default")
    set_default(false)
    add_files("test/future_void.cpp", "src/core/ThreadPool.cpp", "src/core/ThreadUtils.cpp")
    add_deps("deps_for_test")

target("test_task")
    set_kind("binary")
    add_tests("default")
    set_default(false)
    add_files("test/task.cpp", "src/core/ThreadPool.cpp", "src/core/ThreadUtils.cpp", "src/core/format_exception.cpp")
    add_deps("deps_for_test")

target("test_task_group")
    set_kind("binary")
    add_tests("default")
    set_default(false)
    add_files("test/task_group.cpp", "src/core/ThreadPool.cpp", "src/core/ThreadUtils.cpp", "src/core/format_exception.cpp")
    add_deps("deps_for_test")


