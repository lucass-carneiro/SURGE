#version 460 core

layout(location = 0) in vec3 vtx_pos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() { gl_Position = projection * view * model * vec4(vtx_pos, 1.0); }