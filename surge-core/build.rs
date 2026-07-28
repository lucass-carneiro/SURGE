use std::env;
use std::process::Command;

fn glslang(src: &str, dst: &str) -> bool {
    let glslang = Command::new("glslang")
        .arg("-V")
        .arg(&src)
        .arg("-o")
        .arg(&dst)
        .output();

    match glslang {
        Ok(o) => {
            if !o.status.success() {
                println!(
                    "Failed to compile glsl shader {}. glslang return exit code {}",
                    src,
                    o.status.code().unwrap()
                );
                println!("{}", String::from_utf8(o.stdout).unwrap());
                false
            } else {
                true
            }
        }
        Err(e) => {
            println!(
                "Failed to compile glsl shader {}. Unable to execute glslang: {}",
                src, e
            );
            false
        }
    }
}

fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();

    println!("cargo::rerun-if-changed=src/vulkan/sprite_database/shaders/sprite.vert");
    println!("cargo::rerun-if-changed=src/vulkan/sprite_database/shaders/sprite.frag");

    if !glslang(
        "src/vulkan/sprite_database/shaders/sprite.vert",
        &format!("{out_dir}/sprite_vert.spv"),
    ) {
        std::process::exit(1);
    }

    if !glslang(
        "src/vulkan/sprite_database/shaders/sprite.frag",
        &format!("{out_dir}/sprite_frag.spv"),
    ) {
        std::process::exit(1);
    }
}
