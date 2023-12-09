#version 450

#extension GL_GOOGLE_include_directive : enable

#include "device.h"

layout(location = 0) in vec2 v_uv;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(v_uv, 1.0, 1.0);
}