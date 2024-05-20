#version 460 core

#extension GL_ARB_bindless_texture : require

layout(std430, binding = 4) readonly buffer ssbo2 { float alphas[]; };
layout(std430, binding = 5) readonly buffer ssbo3 { sampler2D textures[]; };

in VS_OUT {
  vec2 uv_coords;
  flat int instance_ID;
}
fs_in;

out vec4 fragment_color;

void main() {
  const sampler2D texture_sampler = textures[fs_in.instance_ID];

  const vec4 texture_color = texture(texture_sampler, fs_in.uv_coords);
  const vec4 alpha_mod = vec4(1.0, 1.0, 1.0, alphas[fs_in.instance_ID]);

  const vec4 final_color = texture_color * alpha_mod;

  // Alpha blending is disabled
  if (final_color.a < 1.0) {
    discard;
  } else {
    fragment_color = texture_color * alpha_mod;
  }
}