#version 460 core

#extension GL_ARB_bindless_texture : require

out vec4 fragment_color;

in VS_OUT {
  vec2 uv_coords;
  flat int instance_ID;
}
fs_in;

uniform float alphas[16];

layout(bindless_sampler) uniform sampler2D textures[16];

void main() {
  const sampler2D texture_sampler = textures[fs_in.instance_ID];

  const vec4 texture_color = texture(texture_sampler, fs_in.uv_coords);
  const vec4 alpha_mod = vec4(1.0, 1.0, 1.0, alphas[fs_in.instance_ID]);

  fragment_color = texture_color * alpha_mod;
}