#version 450

#extension GL_GOOGLE_include_directive : enable

#include "device.h"
#include "vertex_info.h"

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texcoords;

layout (location = 0) out vec2 v_texcoords;

layout (binding = BINDING_UBO) uniform UniformBufferObject_
{
    UniformBufferObject ubo;
};

void main()
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(in_position, 1.0);
    v_texcoords = in_texcoords;
}
