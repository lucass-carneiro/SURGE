#version 460 core

out vec4 fragment_color;

in VS_OUT { vec2 txt_pos; }
fs_in;

uniform sampler2D txt_0;

uniform bool h_flip;
uniform bool v_flip;

uniform vec2 ds;
uniform vec2 r0;
uniform vec2 dims;

mat3 subimage_matrix() {
  const float du = ds.x;
  const float dv = ds.y;

  const float x0 = r0.x;
  const float y0 = r0.y;

  const float w = dims.x;
  const float h = dims.y;

  const float A = du * w;
  const float B = dv * h;
  const float C = du * x0;
  const float D = 1 - dv * (y0 + h);

  return mat3(A, 0, 0, 0, B, 0, C, D, 1);
}

mat3 sprite_h_flip_matrix() { return mat3(-1, 0, 0, 0, 1, 0, 1, 0, 1); }

mat3 sprite_v_flip_matrix() { return mat3(1, 0, 0, 0, -1, 0, 0, 1, 1); }

vec2 get_sprite_coord() {
  mat3 M = subimage_matrix();

  const vec3 ext_txt_pos = vec3(fs_in.txt_pos, 1.0);

  if (h_flip) {
    M = M * sprite_h_flip_matrix();
  }

  if (v_flip) {
    M = M * sprite_v_flip_matrix();
  }

  return (M * ext_txt_pos).xy;
}

void main() { fragment_color = texture(txt_0, get_sprite_coord()); }