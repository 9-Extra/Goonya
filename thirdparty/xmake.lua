-- OpenGL加载库
target("glad")
    set_kind("object")
    add_files("glad/src/glad.c")
    add_includedirs("glad/include", {public = true})

-- STB库组合，包括图像加载和保存
target("stb")
    set_kind("object")
    add_files("stb/src/stb.c")
    if is_plat("windows") then
        add_defines("_CRT_SECURE_NO_WARNINGS")
    end 
    add_includedirs("stb/include", {public = true})
