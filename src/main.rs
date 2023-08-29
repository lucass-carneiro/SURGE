#![feature(thread_id_value)]

use glfw::Context;
use mimalloc::MiMalloc;

#[global_allocator]
static GLOBAL: MiMalloc = MiMalloc;

#[macro_use]
mod logging;
mod cli;
mod renderer;
mod shader;
mod window;

fn main() {
    /********
     * Logo *
     ********/
    cli::draw_logo();

    /******************
     * Parse CLI args *
     ******************/
    let config_file = cli::parse_cfg().unwrap();

    /****************************
     * Init window and renderer *
     ****************************/
    let (mut glfw_ctx, mut window, event_reciever) = window::init(&config_file).unwrap();
    let clear_color = renderer::get_clear_color(&config_file).unwrap();

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
        renderer::clear(&clear_color);

        // Call module draw

        // Present rendering
        window.swap_buffers();

        // Refresh cached HR key
        hr_key_old_state = window.get_key(glfw::Key::F5) == glfw::Action::Press
            && window.get_key(glfw::Key::LeftControl) == glfw::Action::Press;
    }
}
