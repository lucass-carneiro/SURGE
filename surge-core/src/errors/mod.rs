use libloading;
use std::io;
use thiserror::Error;
use toml;

#[derive(Debug, Error)]
pub enum ModuleError {
    #[error("Unable to load SURGE module {name}: {io_error}")]
    IoError { name: String, io_error: io::Error },

    #[error("Unable to load SURGE module {name}: {lib_error}")]
    LibLoadingError {
        name: String,
        lib_error: libloading::Error,
    },
}

#[derive(Debug, Error)]
pub enum ConfigError {
    #[error("Unable to load SURGE configuration file {name}: {io_error}")]
    IoError { name: String, io_error: io::Error },

    #[error("Unable to parse SURGE configuration file {name}")]
    ParseError {
        name: String,
        error: toml::de::Error,
    },
}
