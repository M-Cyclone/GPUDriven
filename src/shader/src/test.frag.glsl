#version 450

#extension GL_GOOGLE_include_directive : enable

#include "device.h"

layout (location = OUT_COLOR_LOCATION) out vec4 out_color;

layout (location = 0) in vec3 v_color;

void main()
{
    out_color = vec4(v_color, 1.0);
}