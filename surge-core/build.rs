use std::process::Command;

fn main() {
    let sprite_vert_compile = Command::new("glslang")
        .arg("-V")
        .arg("src/vulkan/sprite_database/shaders/sprite.vert")
        .arg("-o")
        .arg("src/vulkan/sprite_database/shaders/sprite.vert.spv")
        .output()
        .expect("Failed to compile sprite database vertex shader");

    let sprite_frag_compile = Command::new("glslang")
        .arg("-V")
        .arg("src/vulkan/sprite_database/shaders/sprite.frag")
        .arg("-o")
        .arg("src/vulkan/sprite_database/shaders/sprite.frag.spv")
        .output()
        .expect("Failed to compile sprite database fragment shader");

    if sprite_vert_compile.status.success() {
        println!("{}", String::from_utf8_lossy(&sprite_vert_compile.stdout));
    } else {
        eprintln!(
            "Command failed: {}",
            String::from_utf8_lossy(&sprite_vert_compile.stdout)
        );
        std::process::exit(1);
    }

    if sprite_frag_compile.status.success() {
        println!("{}", String::from_utf8_lossy(&sprite_frag_compile.stdout));
    } else {
        eprintln!(
            "Command failed: {}",
            String::from_utf8_lossy(&sprite_frag_compile.stderr)
        );
        std::process::exit(1);
    }
}
