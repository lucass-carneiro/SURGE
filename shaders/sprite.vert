#version 460 core

#extension GL_ARB_bindless_texture : require

layout(location = 0) in vec3 vtx_pos;
layout(location = 1) in vec2 uv_coords;
layout(binding = 2, std430) readonly buffer ssbo1 { mat4 models[]; };

out VS_OUT {
  vec2 uv_coords;
  flat int instance_ID;
}
vs_out;

uniform mat4 projection;
uniform mat4 view;
// uniform mat4 models[16];

void main() {
  gl_Position = projection * view * models[gl_InstanceID] * vec4(vtx_pos, 1.0);

  vs_out.uv_coords = uv_coords;
  vs_out.instance_ID = gl_InstanceID;
}