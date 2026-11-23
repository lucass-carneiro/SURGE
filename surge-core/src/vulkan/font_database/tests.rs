use super::*;

/// `FrameGlobals` and `InstanceRecord` are written byte-for-byte into buffers the font shaders read
/// under a fixed std140/std430 layout (font.vert). This pins the Rust-side size/align to those
/// layouts so a reordered or added field is caught here instead of silently desyncing from the
/// GLSL. The font `InstanceRecord` drops the sprite's trailing `material` field (single atlas).
#[test]
fn gpu_struct_layout_matches_glsl() {
    // std140 uniform block: 2x mat4, each 64 bytes / 16-byte aligned.
    assert_eq!(size_of::<FrameGlobals>(), 128);

    // std430 array element: mat4 (64) + vec4 (16) + vec4 (16), already a multiple of the struct's
    // 16-byte alignment, so no tail padding.
    assert_eq!(size_of::<InstanceRecord>(), 96);
    assert_eq!(std::mem::align_of::<InstanceRecord>(), 16);
}
