#version 460 core
out vec4 o_fragment_color;

in vec2 txt_pos;

uniform sampler2D txt_0;

void main() {
    o_fragment_color = texture(txt_0, txt_pos);
} 