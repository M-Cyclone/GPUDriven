#version 450

#extension GL_GOOGLE_include_directive : enable

#include "device.h"

layout(location = 0) in vec3 v_frag_color;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(v_frag_color, 1.0);
}