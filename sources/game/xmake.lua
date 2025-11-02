target("GDemo")
    set_kind("binary")
    add_files("src/**.cpp")
    add_includedirs("src")
    add_deps("GRuntime")
    add_rpathdirs("${ORIGIN}")
    