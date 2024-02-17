#version 460 core

#extension GL_ARB_bindless_texture : require

layout(std430, binding = 3) readonly buffer ssbo3 { sampler2D textures[]; };

in VS_OUT {
  vec2 uv_coords;
  flat int instance_ID;
}
fs_in;

out vec4 fragment_color;

uniform vec4 color;

void main() {
  const mat3 v_flip = mat3(1, 0, 0, 0, -1, 0, 0, 1, 1);

  const vec2 uv_coords = (v_flip * vec3(fs_in.uv_coords, 1.0)).xy;

  const sampler2D texture_sampler = textures[fs_in.instance_ID];

  const vec4 sampled = vec4(1.0, 1.0, 1.0, texture(texture_sampler, uv_coords).r);

  fragment_color = color * sampled;
}