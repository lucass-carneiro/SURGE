use crate::errors::ConfigError;
use log;
use serde::Deserialize;
use std::fs;
use std::path::Path;
use toml;

#[derive(Debug, Deserialize)]
pub struct WindowConfig {
    pub name: String,
    pub windowed: bool,
    pub allow_resizes: bool,
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
pub struct EngineConfig {
    pub window: WindowConfig,
    pub resolution: ResolutionConfig,
    pub renderer: RendererConfig,
    pub clear_color: ClearColorConfig,
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

    return Ok(config);
}
