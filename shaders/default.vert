#version 460 core
layout (location = 0) in vec3 i_vertex_positions;
layout (location = 1) in vec2 i_texture_coordinates;

out vec2 texture_coordinates;

void main() {
    gl_Position = vec4(i_vertex_positions, 1.0);
    texture_coordinates = i_texture_coordinates;
}