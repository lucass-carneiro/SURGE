#![feature(thread_id_value)]

use surge_core::chrono;
use surge_core::glfw;
use surge_core::glfw::Context;

use surge_core::cli::draw_logo;
use surge_core::cli::parse_cfg;

use surge_core::log_info;

use surge_core::renderer::clear;
use surge_core::renderer::get_clear_color;

use surge_core::window::init;

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

    /***********************
     * Main Loop variables *
     ***********************/
    let mut hr_key_old_state = window.get_key(glfw::Key::F5) == glfw::Action::Press
        && window.get_key(glfw::Key::LeftControl) == glfw::Action::Press;

    /*************
     * Main Loop *
     *************/
    // https://github.com/bwasty/learn-opengl-rs/blob/master/src/_1_getting_started/_1_1_hello_window.rs
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

        // Clear buffers
        clear(&clear_color);

        // Call module draw

        // Present rendering
        window.swap_buffers();

        // Refresh cached HR key
        hr_key_old_state = window.get_key(glfw::Key::F5) == glfw::Action::Press
            && window.get_key(glfw::Key::LeftControl) == glfw::Action::Press;
    }
}
