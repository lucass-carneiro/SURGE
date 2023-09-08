#![feature(thread_id_value)]

use mimalloc::MiMalloc;

#[global_allocator]
static GLOBAL: MiMalloc = MiMalloc;

use surge_core::glfw;
use surge_core::glfw::Context;

use surge_core::cli::draw_logo;
use surge_core::cli::parse_cfg;

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
    let (curr_mod, curr_mod_name) = module::load_from_config(&config_file).unwrap();
    module::checked_on_load(&curr_mod).unwrap();

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

        // Handle Hot Reloading
        if should_hr {
            module::reload(&curr_mod_name);
        }

        // Call module update
        unsafe {
            curr_mod.update(dt_timer.elapsed().as_secs_f64());
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

    module::unload(curr_mod).unwrap();
}
