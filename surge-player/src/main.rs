extern crate surge_core as sc;

#[cfg(feature = "hot_reloading")]
extern crate surge_hot_reload;

#[cfg(not(feature = "hot_reloading"))]
extern crate surge_mod_default;

mod cli;

pub fn main() {
    /********
     * Logo *
     ********/
    cli::print_logo();

    /*********************
     * Parse config file *
     *********************/
    cli::init_env_logger();

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
