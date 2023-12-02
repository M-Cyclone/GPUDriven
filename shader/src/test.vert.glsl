#version 450

#extension GL_GOOGLE_include_directive : enable

#include "device.h"

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_color;

layout(location = 0) out vec3 v_frag_color;

void main() {
    gl_Position  = vec4(a_pos, 1.0);
    v_frag_color = a_color;
}
