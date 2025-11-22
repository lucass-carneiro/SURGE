extern crate surge_core as sc;

#[cfg(feature = "hot_reloading")]
extern crate surge_hot_reload;

#[cfg(not(feature = "hot_reloading"))]
extern crate surge_mod_default;

use env_logger;
use log;

fn init_env_logger() {
    use env_logger::Builder;
    use log::LevelFilter;

    let mut builder = Builder::from_default_env();
    builder.filter_level(LevelFilter::Trace);
    builder.init();
}

fn print_logo() {
    let logo = r"    d888888o.   8 8888      88 8 888888888o.        ,o888888o.    8 8888888888
  .`8888:' `88. 8 8888      88 8 8888    `88.      8888     `88.  8 8888
  8.`8888.   Y8 8 8888      88 8 8888     `88   ,8 8888       `8. 8 8888
  `8.`8888.     8 8888      88 8 8888     ,88   88 8888           8 8888
   `8.`8888.    8 8888      88 8 8888.   ,88'   88 8888           8 888888888888
    `8.`8888.   8 8888      88 8 888888888P'    88 8888           8 8888
     `8.`8888.  8 8888      88 8 8888`8b        88 8888   8888888 8 8888
 8b   `8.`8888. ` 8888     ,8P 8 8888 `8b.      `8 8888       .8' 8 8888
 `8b.  ;8.`8888   8888   ,d8P  8 8888   `8b.       8888     ,88'  8 8888
  `Y8888P ,88P'    `Y88888P'   8 8888     `88.      `8888888P'    8 888888888888";
    println!("\x1b[1;38;2;220;20;60m{logo}\x1b[m");
}

pub fn main() {
    /********
     * Logo *
     ********/
    print_logo();

    /*********************
     * Parse config file *
     *********************/
    init_env_logger();

    // Parse config
    let engine_config = sc::config::parse_config("config.toml").unwrap();

    /***************
     * Init window *
     ***************/

    /***********************
     * Init render backend *
     ***********************/

    /*********************
     * Load First module *
     *********************/

    /***********************
     * Main Loop variables *
     ***********************/

    /*************
     * Main Loop *
     *************/
    // Event handling
    // Stop rendering if minimized
    // Rebuild swapchain if necessary
    // Handle hot reloading
    // Call module update
    // Acquire swapchain image
    // Begin command recording
    // Clear screen
    // Call module draw
    // End command recording
    // Submit command buffer
    // Present
    // Refresh HR key state
    // FPS Cap.

    /********************
     * Finalize modules *
     ********************/

    /********************************
     * Finalize window and renderer *
     ********************************/

    #[cfg(feature = "hot_reloading")]
    let lib = surge_hot_reload::HotReloadModule::load_module("target/debug/", "surge_mod_default");
    surge_mod_default::mod_func();
}
