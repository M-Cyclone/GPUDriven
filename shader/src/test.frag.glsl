#version 450

#extension GL_GOOGLE_include_directive : enable

#include "device.h"
#include "vertex_info.h"

layout (location = LOCATION_VERTEX_OUT_COLOR) in vec3 v_color;

layout (location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(v_color, 1.0);
}