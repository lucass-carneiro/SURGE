#version 460 core

#extension GL_ARB_bindless_texture : require

layout(bindless_sampler, location = 4) uniform sampler2D texture_sampler;
layout(bindless_sampler, location = 5) uniform sampler2D depth_texture_sampler;

in VS_OUT { vec2 uv_coords; }
fs_in;

out vec4 fragment_color;

void main() {
  const vec4 texture_color = texture(texture_sampler, fs_in.uv_coords);

  // Alpha discarding
  if (texture_color.a < 0.1) {
    discard;
  } else {
    fragment_color = texture_color;
    gl_FragDepth = texture(depth_texture_sampler, fs_in.uv_coords).r;
  }
}