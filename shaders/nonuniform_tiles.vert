#version 460 core

layout(location = 0) in vec3 vtx_pos;
layout(location = 1) in vec2 txt_pos;

out VS_OUT { vec2 txt_pos; }
vs_out;

uniform mat4 projection;
uniform mat4 view;

uniform vec3 positions[8];
uniform vec3 scales[8];

mat4 translation(vec3 t) { return mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, t.x, t.y, t.z, 1); }
mat4 scale(vec3 s) { return mat4(s.x, 0, 0, 0, 0, s.y, 0, 0, 0, 0, s.z, 0, 0, 0, 0, 1); }

void main() {
  const vec3 p = positions[gl_InstanceID];
  const vec3 s = scales[gl_InstanceID];

  gl_Position = projection * view * translation(p) * scale(s) * vec4(vtx_pos, 1.0);
  vs_out.txt_pos = txt_pos;
}