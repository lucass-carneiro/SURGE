#[cfg(unix)]
mod proc;
#[cfg(target_os = "linux")]
mod remote_app;

use surge_sdk::{SurgeApp, config::StartupAppConfig};

/// Spawns the module binary named by `config` as a child process and connects to it.
#[cfg(target_os = "linux")]
fn spawn_and_connect(config: &StartupAppConfig) -> std::io::Result<Box<dyn SurgeApp>> {
    Ok(Box::new(remote_app::RemoteApp::new(config)?))
}

pub fn main() {
    // Parse config
    let engine_config = surge_sdk::config::parse_config("config.toml").unwrap();

    // Load startup app: spawned as a child process and driven over IPC.
    #[cfg(target_os = "linux")]
    let startup_app = spawn_and_connect(&engine_config.startup_app).unwrap();
    #[cfg(not(target_os = "linux"))]
    let startup_app: Box<dyn SurgeApp> =
        panic!("the module process loader is only supported on Linux");

    surge_player::run(startup_app, engine_config);
}
