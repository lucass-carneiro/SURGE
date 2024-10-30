#version 460 core

#extension GL_ARB_bindless_texture : require

// Sprite Info

struct sprite_info {
  sampler2D texture;
  float color_mod[4];
  float model[16];
  float view[4];
};

// Inputs

layout(location = 0) in vec3 vtx_pos;
layout(location = 1) in vec2 uv_coords;

layout(std140, binding = 2) uniform pv_ubo {
  mat4 projection;
  mat4 view;
};

layout(std430, binding = 3) readonly buffer ssbo1 { sprite_info sprite_infos[]; };

// Output

out VS_OUT {
  vec2 uv_coords;
  flat int instance_ID;
}
vs_out;

// Utility functions

mat4 get_model(uint idx) {
  return mat4(
    sprite_infos[idx].model[0],
    sprite_infos[idx].model[1],
    sprite_infos[idx].model[2],
    sprite_infos[idx].model[3],
    sprite_infos[idx].model[4],
    sprite_infos[idx].model[5],
    sprite_infos[idx].model[6],
    sprite_infos[idx].model[7],
    sprite_infos[idx].model[8],
    sprite_infos[idx].model[9],
    sprite_infos[idx].model[10],
    sprite_infos[idx].model[11],
    sprite_infos[idx].model[12],
    sprite_infos[idx].model[13],
    sprite_infos[idx].model[14],
    sprite_infos[idx].model[15]
  );
}

// Main

void main() {
  vs_out.uv_coords = uv_coords;
  vs_out.instance_ID = gl_InstanceID;
  gl_Position = projection * view * get_model(gl_InstanceID) * vec4(vtx_pos, 1.0);
}