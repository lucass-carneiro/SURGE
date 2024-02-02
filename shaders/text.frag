#version 460 core

#extension GL_ARB_bindless_texture : require

out vec4 fragment_color;

in VS_OUT {
  vec2 uv_coords;
  flat int instance_ID;
}
fs_in;

layout(bindless_sampler) uniform sampler2D textures[16];

void main() {
  const mat3 v_flip = mat3(1, 0, 0, 0, -1, 0, 0, 1, 1);

  const vec2 uv_coords = (v_flip * vec3(fs_in.uv_coords, 1.0)).xy;

  const sampler2D texture_sampler = textures[fs_in.instance_ID];

  const vec4 sampled = vec4(1.0, 1.0, 1.0, texture(texture_sampler, uv_coords).r);

  const vec4 text_color = vec4(1.0, 1.0, 1.0, 1.0);

  fragment_color = text_color * sampled;
}