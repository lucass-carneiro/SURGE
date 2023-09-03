#![feature(thread_id_value)]

use mimalloc::MiMalloc;

#[global_allocator]
static GLOBAL: MiMalloc = MiMalloc;

use surge_core::chrono;
use surge_core::glfw;
use surge_core::glfw::Context;

use surge_core::cli::draw_logo;
use surge_core::cli::parse_cfg;

use surge_core::log_info;

use surge_core::renderer::clear;
use surge_core::renderer::get_clear_color;

use surge_core::window::init;

use surge_core::module;

fn main() {
    /********
     * Logo *
     ********/
    draw_logo();

    /******************
     * Parse CLI args *
     ******************/
    let config_file = parse_cfg().unwrap();

    /****************************
     * Init window and renderer *
     ****************************/
    let (mut glfw_ctx, mut window, _event_reciever) = init(&config_file).unwrap();
    let clear_color = get_clear_color(&config_file).unwrap();

    /*********************
     * Load First module *
     *********************/
    // TODO: First module needs to be loaded from config
    let current_module = module::load("module_default").unwrap();
    module::on_load(&current_module).unwrap();
    let module_update = module::get_update_handle(&current_module).unwrap();

    /***********************
     * Main Loop variables *
     ***********************/
    let mut hr_key_old_state = window.get_key(glfw::Key::F5) == glfw::Action::Press
        && window.get_key(glfw::Key::LeftControl) == glfw::Action::Press;

    let mut dt_timer = std::time::Instant::now();

    /*************
     * Main Loop *
     *************/
    while !window.should_close() {
        glfw_ctx.poll_events();

        // Handle hot reloading
        let should_hr = window.get_key(glfw::Key::F5) == glfw::Action::Press
            && window.get_key(glfw::Key::LeftControl) == glfw::Action::Press
            && hr_key_old_state == false;

        if should_hr {
            log_info!("Hot reload");
        }

        // Call module update
        unsafe {
            module_update(dt_timer.elapsed().as_secs_f64());
        }
        dt_timer = std::time::Instant::now();

        // Clear buffers
        clear(&clear_color);

        // Call module draw

        // Present rendering
        window.swap_buffers();

        // Refresh cached HR key
        hr_key_old_state = window.get_key(glfw::Key::F5) == glfw::Action::Press
            && window.get_key(glfw::Key::LeftControl) == glfw::Action::Press;
    }

    module::unload(current_module).unwrap();
}
