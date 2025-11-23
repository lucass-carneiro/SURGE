extern crate surge_core as sc;

#[cfg(feature = "hot_reloading")]
extern crate surge_hot_reload;
#[cfg(not(feature = "hot_reloading"))]
extern crate surge_mod_default as md;

use std::time::Instant;
use std::{sync::Arc, time::Duration};
use winit::dpi::PhysicalSize;
use winit::{
    event::{Event, WindowEvent},
    event_loop::EventLoop,
    window::{Fullscreen, WindowBuilder},
};

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
    let event_loop = EventLoop::new();
    let primary_monitor_handle = event_loop.primary_monitor();

    let window = match WindowBuilder::new()
        .with_title(engine_config.window.name.clone())
        .with_fullscreen(if engine_config.window.windowed {
            None
        } else {
            Some(Fullscreen::Borderless(primary_monitor_handle))
        })
        .with_resizable(engine_config.window.allow_resizes)
        .with_inner_size(PhysicalSize::new(
            engine_config.resolution.width,
            engine_config.resolution.height,
        ))
        .build(&event_loop)
    {
        Ok(o) => Arc::new(o),
        Err(e) => {
            log::error!("Unable to create SURGE window: {}", e);
            return;
        }
    };

    /***********************
     * Init render backend *
     ***********************/
    let vulkan_context = sc::vulkan::VulkanContext::new(&event_loop).unwrap();

    /*********************
     * Load First module *
     *********************/
    md::on_load();

    /***********************
     * Main Loop variables *
     ***********************/
    let desired_loop_time = 1.0 / (engine_config.renderer.fps_cap as f64);

    /*************
     * Main Loop *
     *************/
    event_loop.run(move |event, _, control_flow| {
        let loop_timer = Instant::now();

        control_flow.set_poll();

        // Event handling
        match event {
            // Window close
            Event::WindowEvent {
                event: WindowEvent::CloseRequested,
                ..
            } => {
                control_flow.set_exit();
            }

            // Engine shutdown
            Event::LoopDestroyed => {
                log::info!("Closing SURGE window");

                /********************
                 * Finalize modules *
                 ********************/
                md::on_unload();

                /********************************
                 * Finalize window and renderer *
                 ********************************/
                log::info!("TODO: Finalize renderer");
            }

            _ => (),
        }

        // Stop rendering if minimized
        // Rebuild swapchain if necessary
        // Handle hot reloading
        // Call module update
        md::update();

        // Acquire swapchain image
        // Begin command recording
        // Clear screen

        // Call module draw
        md::draw();

        // End command recording
        // Submit command buffer
        // Present
        // Refresh HR key state

        // FPS Cap.
        let loop_time = loop_timer.elapsed().as_secs_f64();

        if engine_config.renderer.cap_fps && (loop_time > desired_loop_time) {
            std::thread::sleep(Duration::from_secs_f64(loop_time - desired_loop_time));
        }
    });
}
