#version 460 core

#extension GL_ARB_bindless_texture : require

// Sprite Info

struct sprite_info {
  sampler2D texture;
  float alpha;
  float model[16];
  float view[4];
};

// Inputs

layout(std430, binding = 3) readonly buffer ssbo1 { sprite_info sprite_infos[]; };

in VS_OUT {
  vec2 uv_coords;
  flat int instance_ID;
}
fs_in;

// Outputs

out vec4 fragment_color;

// Utility functions

float get_alpha(uint idx) {
  return sprite_infos[idx].alpha;
}

sampler2D get_texture(uint idx) {
  return sprite_infos[idx].texture;
}

vec4 get_view(uint idx) {
  return vec4(
    sprite_infos[idx].view[0],
    sprite_infos[idx].view[1],
    sprite_infos[idx].view[2],
    sprite_infos[idx].view[3]
  );
}

// Main

void main() {
  // Recover texture
  const sampler2D texture_sampler = get_texture(fs_in.instance_ID);

  // Crop to image view
  const vec4 iv = get_view(fs_in.instance_ID);
  const mat3 image_view_mat = mat3(iv[0], 0, 0, 0, iv[1], 0, iv[2], iv[3], 1);
  const vec2 cropped_uv_coords = (image_view_mat * vec3(fs_in.uv_coords, 1.0)).xy;

  // Obtain color from texture
  const vec4 texture_color = texture(texture_sampler, cropped_uv_coords);
  
  // Obtain sprite alpha
  const vec4 alpha_mod = vec4(1.0, 1.0, 1.0, get_alpha(fs_in.instance_ID));

  // Final color
  const vec4 final_color = texture_color * alpha_mod;
  
  // Alpha discarding
  if (final_color.a < 0.1) {
    discard;
  } else {
    fragment_color = final_color;
  }
}