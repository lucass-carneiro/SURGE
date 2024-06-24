#version 460 core

#extension GL_ARB_bindless_texture : require

layout(location = 0) in vec3 vtx_pos;
layout(location = 1) in vec2 uv_coords;

layout(std140, binding = 2) uniform pv_ubo {
  mat4 projection;
  mat4 view;
};

layout(location = 3) uniform mat4 model;

out VS_OUT { vec2 uv_coords; }
vs_out;

void main() {
  gl_Position = projection * view * model * vec4(vtx_pos, 1.0);
  vs_out.uv_coords = uv_coords;
}