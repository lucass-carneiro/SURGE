#version 450

layout (push_constant) uniform constants {
    mat4 projection;
    mat4 view;
    mat4 model;
} WorldMatrices;

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

    gl_Position = WorldMatrices.projection * WorldMatrices.view * WorldMatrices.model * vec4(positions[gl_VertexIndex], 1.0f);
}
