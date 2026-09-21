use std::env;
use std::process::Command;

fn glslang(src: &str, dst: &str) -> Result<(), String> {
    let o = Command::new("glslang")
        .args(["-V", "--target-env", "vulkan1.3"])
        .arg(src)
        .arg("-o")
        .arg(dst)
        .output()
        .map_err(|e| format!("Failed to compile glsl shader {src}. Unable to execute glslang: {e}"))?;

    if o.status.success() {
        return Ok(());
    }

    let status = match o.status.code() {
        Some(code) => format!("exit code {code}"),
        None => "no exit code (terminated by a signal)".to_string(),
    };
    Err(format!(
        "Failed to compile glsl shader {src}. glslang returned {status}\n{}{}",
        String::from_utf8_lossy(&o.stdout),
        String::from_utf8_lossy(&o.stderr)
    ))
}

fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();

    let shaders = [
        ("src/vulkan/sprite_database/shaders/sprite.vert", "sprite_vert.spv"),
        ("src/vulkan/sprite_database/shaders/sprite.frag", "sprite_frag.spv"),
    ];

    for (src, spv) in shaders {
        println!("cargo::rerun-if-changed={src}");
        if let Err(msg) = glslang(src, &format!("{out_dir}/{spv}")) {
            // Written to stderr, which cargo always shows when a build script fails
            // (unlike stdout, which needs -vv).
            eprintln!("{msg}");
            std::process::exit(1);
        }
    }
}
