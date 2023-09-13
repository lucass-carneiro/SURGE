use crate::log_error;
use crate::log_info;
use crate::value_or_error;

#[cfg(feature = "log_color")]
pub fn draw_logo() {
    print!("\x1b[1;38;2;220;20;60m");
    println!("   d888888o.   8 8888      88 8 888888888o.        ,o888888o.    8 8888888888   ");
    println!(" .`8888:' `88. 8 8888      88 8 8888    `88.      8888     `88.  8 8888        ");
    println!(" 8.`8888.   Y8 8 8888      88 8 8888     `88   ,8 8888       `8. 8 8888        ");
    println!(" `8.`8888.     8 8888      88 8 8888     ,88   88 8888           8 8888        ");
    println!("  `8.`8888.    8 8888      88 8 8888.   ,88'   88 8888           8 888888888888");
    println!("   `8.`8888.   8 8888      88 8 888888888P'    88 8888           8 8888        ");
    println!("    `8.`8888.  8 8888      88 8 8888`8b        88 8888   8888888 8 8888        ");
    println!("8b   `8.`8888. ` 8888     ,8P 8 8888 `8b.      `8 8888       .8' 8 8888        ");
    println!("`8b.  ;8.`8888   8888   ,d8P  8 8888   `8b.       8888     ,88'  8 8888        ");
    println!(
        " `Y8888P ,88P'    `Y88888P'   8 8888     `88.      `8888888P'    8 888888888888\x1b[m"
    );
}

#[cfg(not(feature = "log_color"))]
pub fn draw_logo() {
    println!("   d888888o.   8 8888      88 8 888888888o.        ,o888888o.    8 8888888888   ");
    println!(" .`8888:' `88. 8 8888      88 8 8888    `88.      8888     `88.  8 8888        ");
    println!(" 8.`8888.   Y8 8 8888      88 8 8888     `88   ,8 8888       `8. 8 8888        ");
    println!(" `8.`8888.     8 8888      88 8 8888     ,88   88 8888           8 8888        ");
    println!("  `8.`8888.    8 8888      88 8 8888.   ,88'   88 8888           8 888888888888");
    println!("   `8.`8888.   8 8888      88 8 888888888P'    88 8888           8 8888        ");
    println!("    `8.`8888.  8 8888      88 8 8888`8b        88 8888   8888888 8 8888        ");
    println!("8b   `8.`8888. ` 8888     ,8P 8 8888 `8b.      `8 8888       .8' 8 8888        ");
    println!("`8b.  ;8.`8888   8888   ,d8P  8 8888   `8b.       8888     ,88'  8 8888        ");
    println!(" `Y8888P ,88P'    `Y88888P'   8 8888     `88.      `8888888P'    8 888888888888");
}

#[derive(Debug)]
pub enum ConfigFileError {
    FileNotLoaded,
    FileNotParsed,
    ExePathNotFound,
}

pub fn parse_cfg() -> Result<Vec<yaml_rust::Yaml>, ConfigFileError> {
    use std::fs;
    use yaml_rust::YamlLoader;

    log_info!("Parsing config file");

    let mut file_path = match std::env::current_dir() {
        Ok(o) => o,
        Err(e) => {
            log_error!("Failed to get current exe path: {}", e);
            return Err(ConfigFileError::ExePathNotFound);
        }
    };
    file_path.push("config.yaml");

    let file_string = value_or_error!(
        fs::read_to_string(file_path),
        ConfigFileError::FileNotLoaded,
        "Unable to load file \"config.yaml\""
    );

    let config_file = value_or_error!(
        YamlLoader::load_from_str(&file_string),
        ConfigFileError::FileNotParsed,
        "Unable to parse \"config.yaml\""
    );

    return Ok(config_file);
}
