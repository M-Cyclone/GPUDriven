#version 450

#extension GL_GOOGLE_include_directive : enable

#include "device.h"
#include "vertex_info.h"

layout (location = LOCATION_VERTEX_IN_POSITION) in vec2 in_position;
layout (location = LOCATION_VERTEX_IN_COLOR)    in vec3 in_color;

layout (location = LOCATION_VERTEX_OUT_COLOR) out vec3 v_color;

void main()
{
    gl_Position = vec4(in_position, 0.0, 1.0);
    v_color     = in_color;
}
