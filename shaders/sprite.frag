#version 460 core

out vec4 fragment_color;

in VS_OUT {
    vec2 txt_pos;
} fs_in;

uniform sampler2D txt_0;

uniform ivec4 sheet_coords;

vec2 get_sprite_coord() {
  const int i = sheet_coords.x;
  const int j = sheet_coords.y;
  const int sw = sheet_coords.z;
  const int sh = sheet_coords.w;
  
  // Show the whole sprite sheet
  if (i < 0 || j < 0) {
    return fs_in.txt_pos;
  } else {
    // Step 1: Compute steps
    const float du = 1.0/sw;
    const float dv = 1.0/sh;

    // Step 1: Extended fs_in.txt_pos
    const vec3 ext_txt_pos = vec3(fs_in.txt_pos, 1.0);

    // Step 2: Build transformation matrix
    const mat3 M = mat3(
      du, 0, 0,
      0, dv, 0,
      j * du, (sh - 1 - i) * dv, 1
    );

    // Step 3: Transform vertex coordinates
    const vec3 sprite_coord = M * ext_txt_pos;
    
    // Step 4: Downgrade dimention and return
    return sprite_coord.xy;
  }
}

void main() {
  fragment_color = texture(txt_0, get_sprite_coord());
} 