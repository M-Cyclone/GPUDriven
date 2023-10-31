set_project("gpu_driven")

set_arch("x64")
set_warnings("all")
set_languages("cxx20")
set_toolchains("clang")

add_rules("mode.debug", "mode.releasedbg", "mode.release")

add_requires("vulkansdk")
add_requires("libsdl")
add_requires("glm")
add_requires("spdlog")
add_requires("stb")
add_requires("vulkan-memory-allocator")
-- add_requires("eastl")
-- add_requires("mimalloc")

target("gpu_driven")
    set_default(true)
    set_kind("binary")
    
    add_cxflags("-Wall", "-Werror", "-ffast-math")

    add_packages("vulkansdk")
    add_packages("libsdl")
    add_packages("glm")
    add_packages("spdlog")
    add_packages("stb")
    add_packages("vulkan-memory-allocator")
    -- add_packages("eastl")
    -- add_packages("mimalloc")
    
    add_includedirs("src")
    add_files(
        "src/core/*.cpp",
        "src/extern_impl/*.cpp",
        "src/vk/*.cpp")    
    add_headerfiles(
        "src/core/*.h",
        "src/shader/header/*.h",
        "src/utils/*.h",
        "src/vk/*.h")

    set_rundir("$(projectdir)")
