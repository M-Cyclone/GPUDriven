add_requires("vulkansdk")
add_requires("glfw")
add_requires("glm")
add_requires("spdlog")
add_requires("stb")
add_requires("vulkan-memory-allocator")
-- add_requires("eastl")
-- add_requires("mimalloc")

BuildProject({
    projectName = "GPU-Driven",
    projectType = "binary",
    debugEvent = function()
        add_defines("_DEBUG")
    end,
    releaseEvent = function()
        add_defines("NDEBUG")
    end,
    exception = true
})

add_defines(
    "_XM_NO_INTRINSICS_=1",
    "FMT_DEPRECATED=/* deprecated */",
    "NOMINMAX",
    "UNICODE",
    "m128_f32=vector4_f32",
    "m128_u32=vector4_u32",
    "USE_VULKAN_VALIDATION_LAYER"
)

add_packages("vulkansdk")
add_packages("glfw")
add_packages("glm")
add_packages("spdlog")
add_packages("stb")
add_packages("vulkan-memory-allocator")
-- add_packages("eastl")
-- add_packages("mimalloc")

add_includedirs("./")
add_files(
    "./core/*.cpp",
    "./extern_impl/*.cpp",
    "./graphics/*.cpp",
    "./graphics/bindable/*.cpp",
    -- "./graphics/drawable/*.cpp",
    "./graphics/resource/*.cpp",
    "./graphics/vulkan_helper/*.cpp",
    "./utils/*.cpp"
)
add_headerfiles(
    "./core/*.h",
    "./shader_header/*.h",
    "./graphics/*.h",
    "./graphics/bindable/*.h",
    "./graphics/drawable/*.h",
    "./graphics/resource/*.h",
    "./graphics/vulkan_helper/*.h",
    "./utils/*.h"
)

set_rundir("$(projectdir)")
