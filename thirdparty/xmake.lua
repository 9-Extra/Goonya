-- OpenGL加载库
target("glad")
    set_kind("static")
    add_files("glad/src/glad.c")
    add_includedirs("glad/include", {public = true})

-- STB库组合，包括图像加载和保存
target("stb")
    set_kind("shared")
    add_files("stb/src/stb.c")
    add_includedirs("stb/include", {public = true})
