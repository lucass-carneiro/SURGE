#version 460 core

#extension GL_ARB_bindless_texture : require

layout(location = 0) in vec3 vtx_pos;
layout(location = 1) in vec2 uv_coords;

layout(std140, binding = 2) uniform pv_ubo {
  mat4 projection;
  mat4 view;
};

layout(std430, binding = 3) readonly buffer ssbo1 { mat4 models[]; };

out VS_OUT {
  vec2 uv_coords;
  flat int instance_ID;
}
vs_out;

void main() {
  gl_Position = projection * view * models[gl_InstanceID] * vec4(vtx_pos, 1.0);

  vs_out.uv_coords = uv_coords;
  vs_out.instance_ID = gl_InstanceID;
}