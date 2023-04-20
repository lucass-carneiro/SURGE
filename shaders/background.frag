#version 460 core

out vec4 fragment_color;

in VS_OUT {
    vec2 txt_pos;
} fs_in;

uniform sampler2D txt_0;

void main() {
  fragment_color = texture(txt_0, fs_in.txt_pos);
} 