#[cfg(any(target_os = "linux", windows))]
mod proc;
#[cfg(any(target_os = "linux", windows))]
mod remote_app;

use surge_sdk::{config::StartupAppConfig, SurgeApp};

/// Spawns the module binary named by `config` as a child process and connects to it.
#[cfg(any(target_os = "linux", windows))]
fn spawn_and_connect(config: &StartupAppConfig) -> std::io::Result<Box<dyn SurgeApp>> {
    Ok(Box::new(remote_app::RemoteApp::new(config)?))
}

pub fn main() {
    // Parse config
    let engine_config = surge_sdk::config::parse_config("config.toml").unwrap();

    // Load startup app: spawned as a child process and driven over IPC.
    #[cfg(any(target_os = "linux", windows))]
    let startup_app = spawn_and_connect(&engine_config.startup_app).unwrap();
    #[cfg(not(any(target_os = "linux", windows)))]
    let startup_app: Box<dyn SurgeApp> =
        panic!("the module process loader is only supported on Linux and Windows");

    surge_player::run(startup_app, engine_config);
}
