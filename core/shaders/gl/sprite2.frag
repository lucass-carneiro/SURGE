#version 460 core

#extension GL_ARB_bindless_texture : require

out vec4 fragment_color;

void main() {
  fragment_color = vec4(1.0, 0.0, 0.0, 1.0);
}