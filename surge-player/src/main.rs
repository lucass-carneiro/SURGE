#[cfg(any(target_os = "linux", windows))]
mod proc;
#[cfg(any(target_os = "linux", windows))]
mod remote_app;

use std::process::ExitCode;
use surge_sdk::{SurgeApp, config::StartupAppConfig};

/// Spawns the module binary named by `config` as a child process and connects to it.
#[cfg(any(target_os = "linux", windows))]
fn spawn_and_connect(config: &StartupAppConfig) -> std::io::Result<Box<dyn SurgeApp>> {
    Ok(Box::new(remote_app::RemoteApp::new(config)?))
}

pub fn main() -> ExitCode {
    // Parse config
    let engine_config = match surge_sdk::config::parse_config("config.toml") {
        Ok(config) => config,
        Err(e) => return surge_player::fail(e),
    };

    // Load startup app: spawned as a child process and driven over IPC.
    #[cfg(any(target_os = "linux", windows))]
    let startup_app = match spawn_and_connect(&engine_config.startup_app) {
        Ok(app) => app,
        Err(e) => {
            return surge_player::fail(format!(
                "Unable to start module {}/{}: {e}",
                engine_config.startup_app.app_folder, engine_config.startup_app.app_name
            ));
        }
    };
    #[cfg(not(any(target_os = "linux", windows)))]
    let startup_app: Box<dyn SurgeApp> = return surge_player::fail(
        "the module process loader is only supported on Linux and Windows",
    );

    surge_player::run(startup_app, engine_config)
}
