#version 460 core
out vec4 o_fragment_color;

in vec2 i_texture_coordinates;

void main() {
    o_fragment_color = vec4(1.0f, 0.5f, 0.2f, 1.0f);
} 