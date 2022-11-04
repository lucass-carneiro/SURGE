#version 460 core

layout (location = 0) in vec3 i_vtx_pos;
layout (location = 1) in vec2 i_txt_pos;

out vec2 txt_pos;

void main() {
    gl_Position = vec4(i_vtx_pos, 1.0);
    txt_pos = i_txt_pos;
}