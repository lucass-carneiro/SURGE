#version 460 core

out vec4 fragment_color;

in VS_OUT {
    vec2 txt_pos;
} fs_in;

uniform sampler2D txt_0;

uniform bool h_flip;
uniform bool v_flip;

mat3 sprite_h_flip_matrix() {
  return mat3(
    -1, 0, 0,
    0, 1, 0,
    1, 0, 1
  );
}

mat3 sprite_v_flip_matrix() {
  return mat3(
    1, 0, 0,
    0, -1, 0,
    0, 1, 1
  );
}

vec2 get_sprite_coord() {
  mat3 M = mat3(1.0);
  const vec3 ext_txt_pos = vec3(fs_in.txt_pos, 1.0);

  if (h_flip) {
    M = M * sprite_h_flip_matrix();
  }

  if (v_flip) {
    M = M * sprite_v_flip_matrix();
  }
  
  return (M * ext_txt_pos).xy;
}

void main() {
  fragment_color = texture(txt_0, get_sprite_coord());
} 