#version 460 core

layout(location = 0) in vec4 vtx_data; // (vec2 pos, vec2 tex_coords)

out vec2 texture_coords;

uniform mat4 projection;

void main() {
  gl_Position = projection * vec4(vtx_data.xy, 0.0, 1.0);
  texture_coords = vtx_data.zw;
}