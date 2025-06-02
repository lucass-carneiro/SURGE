#version 450

#extension GL_EXT_nonuniform_qualifier: require

layout (location = 0) in vec4 in_color_multiplier;
layout (location = 1) in vec2 in_uv_coordinates;
layout (location = 2) in flat uint in_instance_index;

layout (location = 0) out vec4 out_fragment_color;

layout (set = 0, binding = 0) uniform sampler2D sprite_textures[];

void main() {
    out_fragment_color = texture(sprite_textures[nonuniformEXT(in_instance_index)], in_uv_coordinates) * in_color_multiplier;
}
