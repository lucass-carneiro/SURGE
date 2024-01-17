#version 460 core

out vec4 fragment_color;

in VS_OUT { vec2 txt_pos; }
fs_in;

void main() { fragment_color = vec4(0.0, 0.0, 0.0, 1.0); }