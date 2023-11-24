#version 450

#extension GL_GOOGLE_include_directive : enable

#include "device.h"
#include "vertex_info.h"

layout(binding = BINDING_SAMPLER) uniform sampler2D tex_sampler;

layout (location = 0) in vec2 v_texcoords;

layout (location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(texture(tex_sampler, v_texcoords).rgb, 1.0);
    // out_color = vec4(v_texcoords, 0.0, 1.0);
}