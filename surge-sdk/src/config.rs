use log;
use serde::Deserialize;
use std::fs;
use std::io;
use std::path::Path;
use thiserror::Error;
use toml;

#[derive(Debug, Error)]
pub enum ConfigError {
    #[error("Unable to load SURGE configuration file {name}: {io_error}")]
    IoError { name: String, io_error: io::Error },

    #[error("Unable to parse SURGE configuration file {name}")]
    ParseError {
        name: String,
        error: toml::de::Error,
    },

    #[error("Invalid SURGE configuration file {name}: {reason}")]
    InvalidValue { name: String, reason: String },
}

#[derive(Debug, Deserialize)]
pub struct WindowConfig {
    pub name: String,
    pub windowed: bool,
}

#[derive(Debug, Deserialize)]
pub struct ResolutionConfig {
    pub width: u32,
    pub height: u32,
}

#[derive(Debug, Deserialize)]
pub struct RendererConfig {
    pub vsync: bool,
    pub msaa: bool,
    pub cap_fps: bool,
    pub fps_cap: u32,
}

#[derive(Debug, Deserialize)]
pub struct ClearColorConfig {
    pub r: f64,
    pub g: f64,
    pub b: f64,
    pub a: f64,
}

#[derive(Debug, Deserialize)]
pub struct StartupAppConfig {
    pub app_folder: String,
    pub app_name: String,
}

#[derive(Debug, Deserialize)]
pub struct EngineConfig {
    pub window: WindowConfig,
    pub resolution: ResolutionConfig,
    pub renderer: RendererConfig,
    pub clear_color: ClearColorConfig,
    pub startup_app: StartupAppConfig,
}

impl EngineConfig {
    /// Rejects values that would panic or produce an unusable window later on.
    fn validate(&self) -> Result<(), String> {
        if self.resolution.width == 0 || self.resolution.height == 0 {
            return Err(format!(
                "resolution must be non-zero, got {}x{}",
                self.resolution.width, self.resolution.height
            ));
        }

        if self.renderer.cap_fps && self.renderer.fps_cap == 0 {
            return Err("renderer.fps_cap must be greater than 0 when cap_fps is true".to_string());
        }

        Ok(())
    }
}

pub fn parse_config(config_file_path: &str) -> Result<EngineConfig, ConfigError> {
    let file_path = Path::new(config_file_path);

    let content = match fs::read_to_string(file_path) {
        Ok(o) => o,
        Err(e) => {
            log::error!(
                "Unable to read engine config file {}: {}",
                config_file_path,
                e
            );

            return Err(ConfigError::IoError {
                name: config_file_path.to_string(),
                io_error: e,
            });
        }
    };

    let config: EngineConfig = match toml::from_str(&content) {
        Ok(o) => o,
        Err(e) => {
            log::error!(
                "Unable to parse engine config file {}: {}",
                config_file_path,
                e
            );

            return Err(ConfigError::ParseError {
                name: config_file_path.to_string(),
                error: e,
            });
        }
    };

    if let Err(reason) = config.validate() {
        log::error!("Invalid engine config file {}: {}", config_file_path, reason);

        return Err(ConfigError::InvalidValue {
            name: config_file_path.to_string(),
            reason,
        });
    }

    return Ok(config);
}

#[cfg(test)]
mod tests {
    use super::*;

    fn config(width: u32, height: u32, cap_fps: bool, fps_cap: u32) -> EngineConfig {
        toml::from_str(&format!(
            r#"
            [window]
            name = "t"
            windowed = true
            [resolution]
            width = {width}
            height = {height}
            [renderer]
            vsync = false
            msaa = false
            cap_fps = {cap_fps}
            fps_cap = {fps_cap}
            [clear_color]
            r = 0.0
            g = 0.0
            b = 0.0
            a = 1.0
            [startup_app]
            app_folder = "."
            app_name = "x"
            "#
        ))
        .unwrap()
    }

    #[test]
    fn accepts_valid_config() {
        assert!(config(500, 800, true, 60).validate().is_ok());
    }

    #[test]
    fn rejects_zero_fps_cap_when_capping() {
        assert!(config(500, 800, true, 0).validate().is_err());
    }

    #[test]
    fn ignores_zero_fps_cap_when_not_capping() {
        assert!(config(500, 800, false, 0).validate().is_ok());
    }

    #[test]
    fn rejects_zero_resolution() {
        assert!(config(0, 800, false, 60).validate().is_err());
        assert!(config(500, 0, false, 60).validate().is_err());
    }
}
