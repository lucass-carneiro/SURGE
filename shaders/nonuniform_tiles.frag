#version 460 core

out vec4 fragment_color;

in VS_OUT {
  vec2 txt_pos;
  flat int instance_ID;
}
fs_in;

uniform sampler2DArray texture_sampler;

void main() { fragment_color = texture(texture_sampler, vec3(fs_in.txt_pos, fs_in.instance_ID)); }