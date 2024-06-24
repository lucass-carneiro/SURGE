#version 460 core

#extension GL_ARB_bindless_texture : require

layout(std430, binding = 4) readonly buffer ssbo2 { float alphas[]; };
layout(std430, binding = 5) readonly buffer ssbo3 { sampler2D textures[]; };
layout(std430, binding = 6) readonly buffer ssbo4 { vec4 image_views[]; };

in VS_OUT {
  vec2 uv_coords;
  flat int instance_ID;
}
fs_in;

out vec4 fragment_color;

void main() {
  // Recover texture
  const sampler2D texture_sampler = textures[fs_in.instance_ID];

  // Crop to image view
  const vec4 iv = image_views[fs_in.instance_ID];
  const mat3 image_view_mat = mat3(iv[0], 0, 0, 0, iv[1], 0, iv[2], iv[3], 1);
  const vec2 cropped_uv_coords = (image_view_mat * vec3(fs_in.uv_coords, 1.0)).xy;

  // Obtain color from texture
  const vec4 texture_color = texture(texture_sampler, cropped_uv_coords);
  const vec4 alpha_mod = vec4(1.0, 1.0, 1.0, alphas[fs_in.instance_ID]);

  const vec4 final_color = texture_color * alpha_mod;

  // Alpha discarding
  if (final_color.a < 0.1) {
    discard;
  } else {
    fragment_color = texture_color * alpha_mod;
  }
}