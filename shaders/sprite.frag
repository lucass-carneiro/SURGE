#version 460 core

out vec4 fragment_color;

in VS_OUT {
    vec2 txt_pos;
} fs_in;

uniform sampler2D txt_0;

uniform vec2 sheet_set_dimentions;
uniform vec2 sheet_offsets;
uniform vec2 sheet_dimentions;
uniform vec2 sheet_indices;

vec2 get_sprite_coord() {
  const float Sw = sheet_set_dimentions.x;
  const float Sh = sheet_set_dimentions.y;
  
  const float x0 = sheet_offsets.x;
  const float y0 = sheet_offsets.y;

  const float w  = sheet_dimentions.x;
  const float h = sheet_dimentions.y;
  
  const float alpha = sheet_indices.x;
  const float beta = sheet_indices.y;

  // Show the whole sprite sheet
  if (alpha < 0 || beta < 0) {
    return fs_in.txt_pos;
  } else {
    const float A = w / Sw;
    const float B = h / Sh;
    const float C = (x0 + w * beta) / Sw;
    const float D = 1 - (y0 + h * (alpha + 1)) / Sh;

    const mat3 M = mat3(
      A, 0, 0,
      0, B, 0,
      C, D, 1
    ); 

    const vec3 ext_txt_pos = vec3(fs_in.txt_pos, 1.0);
    const vec3 sprite_coord = M * ext_txt_pos;
    return sprite_coord.xy;
  }
}

void main() {
  fragment_color = texture(txt_0, get_sprite_coord());
} 