#version 460 core

out vec4 fragment_color;

in vec2 texture_coords;

uniform sampler2D text_sampler;
uniform vec3 text_color;

void main() {
  const vec4 sampled_color = vec4(1.0, 1.0, 1.0, texture(text_sampler, texture_coords).r);
  const vec4 ext_text_color = vec4(text_color, 1.0);
  fragment_color = ext_text_color * sampled_color;
}