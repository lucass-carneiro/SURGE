#version 460

// Single-channel (R8) glyph atlas: each texel is coverage, not colour. Text colour comes from the
// per-run instance colour; the atlas only modulates its alpha.
layout(set = 1, binding = 0) uniform sampler smp;
layout(set = 1, binding = 1) uniform texture2D atlas;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 out_color;

void main() {
    float coverage = texture(sampler2D(atlas, smp), uv).r;
    out_color = vec4(color.rgb, color.a * coverage);
}
