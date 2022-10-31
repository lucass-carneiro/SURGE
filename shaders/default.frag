#version 460 core
out vec4 o_fragment_color;

in vec2 texture_coordinates;

uniform sampler2D texture_sampler;

void main() {
    o_fragment_color = texture(texture_sampler, texture_coordinates);
} 