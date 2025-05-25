#version 450
#extension GL_EXT_buffer_reference: require

struct SpriteData {
    mat4 model_matrix;
};

layout (buffer_reference, std430) readonly buffer SpriteDataBuffer {
    SpriteData sprite_data[];
};

layout (push_constant) uniform constants {
    mat4 projection;
    mat4 view;
    SpriteDataBuffer sprite_data_buffer;
} PushConstants;

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

    const SpriteData sprite_data = PushConstants.sprite_data_buffer.sprite_data[gl_InstanceIndex];
    const mat4 model = sprite_data.model_matrix;

    gl_Position = PushConstants.projection * PushConstants.view * model * vec4(positions[gl_VertexIndex], 1.0f);
}
