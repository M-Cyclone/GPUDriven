#version 450

#extension GL_GOOGLE_include_directive : enable

#include "device.h"
#include "vertex_info.h"

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;

layout(location = 0) out vec2 v_uv;

layout (binding = BINDING_UBO) uniform UniformBufferObject_
{
    UniformBufferObject ubo;
};

void main()
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(a_pos, 1.0);
    v_uv        = a_uv;
}
