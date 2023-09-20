#version 460 core

layout(location = 0) in vec3 vtx_pos;
layout(location = 1) in vec2 txt_pos;

out VS_OUT { vec2 txt_pos; }
vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
  gl_Position = projection * view * model * vec4(vtx_pos, 1.0);
  vs_out.txt_pos = txt_pos;
}