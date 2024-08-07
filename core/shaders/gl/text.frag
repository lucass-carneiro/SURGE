#version 460 core

#extension GL_ARB_bindless_texture : require

layout(std430, binding = 4) readonly buffer ssbo2 { sampler2D textures[]; };

in VS_OUT {
  vec2 uv_coords;
  flat int instance_ID;
}
fs_in;

out vec4 fragment_color;

layout(location = 5) uniform vec4 color;

void main() {
  const sampler2D texture_sampler = textures[fs_in.instance_ID];

  const vec4 sampled = vec4(1.0, 1.0, 1.0, texture(texture_sampler, fs_in.uv_coords).r);

  fragment_color = color * sampled;
}