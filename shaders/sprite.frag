#version 460 core

#extension GL_ARB_bindless_texture : require

out vec4 fragment_color;

in VS_OUT {
  vec2 uv_coords;
  flat int instance_ID;
}
fs_in;

layout(bindless_sampler) uniform sampler2D textures[8];

void main() { fragment_color = texture(textures[fs_in.instance_ID], fs_in.uv_coords); }