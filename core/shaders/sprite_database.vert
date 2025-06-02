#version 450
#extension GL_EXT_buffer_reference: require

struct SpriteData {
    mat4 model_matrix;
    vec4 color_multiplier;
};

layout (buffer_reference, std430) readonly buffer SpriteDataBuffer {
    SpriteData sprite_data[];
};

layout (push_constant) uniform constants {
    mat4 projection;
    mat4 view;
    SpriteDataBuffer sprite_data_buffer;
} PushConstants;

layout (location = 0) out vec4 out_color_multiplier;
layout (location = 1) out vec2 out_uv_coordinates;
layout (location = 2) out uint out_instance_index;

void main() {
    // Base quad vertices
    const vec3 positions[6] = vec3[6](
    vec3(0.0f, 0.0f, 0.0f), // UL
    vec3(1.0f, 0.0f, 0.0f), // UR
    vec3(1.0f, 1.0f, 0.0f), // LR

    vec3(1.0f, 1.0f, 0.0f), // LR
    vec3(0.0f, 1.0f, 0.0f), // LL
    vec3(0.0f, 0.0f, 0.0f)  // UL
    );

    // Base quad UV coordinates
    const vec2 uv_coordinates[6] = vec2[6](
    vec2(0.0f, 0.0f), // UL
    vec2(1.0f, 0.0f), // UR
    vec2(1.0f, 1.0f), // LR

    vec2(1.0f, 1.0f), // LR
    vec2(0.0f, 1.0f), // LL
    vec2(0.0f, 0.0f)  // UL
    );

    const SpriteData sprite_data = PushConstants.sprite_data_buffer.sprite_data[gl_InstanceIndex];

    gl_Position = PushConstants.projection * PushConstants.view * sprite_data.model_matrix * vec4(positions[gl_VertexIndex], 1.0f);
    out_color_multiplier = sprite_data.color_multiplier;
    out_uv_coordinates = uv_coordinates[gl_VertexIndex];
    out_instance_index = gl_InstanceIndex;
}
