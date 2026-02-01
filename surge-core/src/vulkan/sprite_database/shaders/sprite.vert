#version 460
#extension GL_EXT_buffer_reference: require

layout(set = 0, binding = 0) uniform FrameGlobals {
    mat4 view;
    mat4 proj;
};

struct InstanceRecord {
    mat4 model;
    vec4 color;
    vec4 subtexture_data;
    uint material;
};

layout(buffer_reference, std430) readonly buffer InstanceRecordBuffer {
    InstanceRecord instance_records[];
};

layout(push_constant) uniform constants {
    InstanceRecordBuffer instance_record_buffer;
} PushConstants;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 color;
layout(location = 2) flat out uint material;

vec2 get_subtexture_uv(vec2 original_uv, vec4 subtexture_data) {
    const vec3 extented_uv = vec3(original_uv, 1.0);

    const float sw_ow = subtexture_data[0];
    const float sh_oh = subtexture_data[1];
    const float sx_ow = subtexture_data[2];
    const float sy_oh = subtexture_data[3];

    const mat3 subtexture_matrix = mat3(
        sw_ow, 0.0  , 0.0,
        0.0  , sh_oh, 0.0,
        sx_ow, sy_oh, 1.0
    );

    return (subtexture_matrix * extented_uv).xy;
}

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

    const InstanceRecord instance_record = PushConstants.instance_record_buffer.instance_records[gl_InstanceIndex];

    gl_Position = proj * view * instance_record.model * vec4(positions[gl_VertexIndex], 1.0f);
    uv = get_subtexture_uv(uv_coordinates[gl_VertexIndex], instance_record.subtexture_data);
    color = instance_record.color;
    material = instance_record.material;
}
