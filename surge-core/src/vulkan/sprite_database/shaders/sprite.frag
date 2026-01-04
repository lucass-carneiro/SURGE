#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 1, binding = 0) uniform sampler smp;
layout(set = 1, binding = 1) uniform texture2D textures[];

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;
layout(location = 2) flat in uint material;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(
        sampler2D(
            textures[nonuniformEXT(material)],
            smp
        ),
        uv
    ) * color;
    //out_color = color;
}