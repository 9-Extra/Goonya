add_rules("mode.debug", "mode.release", "mode.releasedbg")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

set_languages("c23", "c++23")
set_warnings("all")

set_encodings("utf-8")

set_targetdir("bin")
set_rundir("bin") 

if is_mode("debug") then
    add_defines("DEBUG")
else 
    add_defines("NDEBUG")
end

includes("sources", "thirdparty")