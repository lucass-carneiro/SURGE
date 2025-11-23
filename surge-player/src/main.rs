extern crate surge_core as sc;

#[cfg(feature = "hot_reloading")]
extern crate surge_hot_reload;
#[cfg(not(feature = "hot_reloading"))]
extern crate surge_mod_default as md;

use sc::vulkan::VulkanContext;
use std::{sync::Arc, time::Duration, time::Instant};
use winit::{
    application::ApplicationHandler,
    dpi::PhysicalSize,
    event::WindowEvent,
    event_loop::{ActiveEventLoop, ControlFlow, EventLoop},
    window::{Fullscreen, Window, WindowId},
};

mod cli;

struct SurgeContext {
    window: Option<Arc<Window>>,
    vulkan_context: Option<VulkanContext>,
    engine_config: sc::config::EngineConfig,
    loop_timer: Instant,
}

impl ApplicationHandler for SurgeContext {
    fn resumed(&mut self, event_loop: &ActiveEventLoop) {
        /***************
         * Init Window *
         ***************/
        if self.window.is_none() && self.vulkan_context.is_none() {
            log::info!("Creating SURGE window");

            let primary_monitor_handle = event_loop.primary_monitor();

            let window_attributes = Window::default_attributes()
                .with_title(self.engine_config.window.name.clone())
                .with_fullscreen(if self.engine_config.window.windowed {
                    None
                } else {
                    Some(Fullscreen::Borderless(primary_monitor_handle))
                })
                .with_resizable(self.engine_config.window.allow_resizes)
                .with_inner_size(PhysicalSize::new(
                    self.engine_config.resolution.width,
                    self.engine_config.resolution.height,
                ));

            match event_loop.create_window(window_attributes) {
                Ok(window) => {
                    self.window = Some(Arc::new(window));

                    /***********************
                     * Init render backend *
                     ***********************/
                    self.vulkan_context =
                        Some(sc::vulkan::VulkanContext::new(&event_loop).unwrap());

                    /*********************
                     * Load First module *
                     *********************/
                    md::on_load();
                }
                Err(e) => {
                    log::error!("Unable to create SURGE window: {}", e);
                    event_loop.exit();
                }
            }
        }
    }

    fn window_event(
        &mut self,
        event_loop: &ActiveEventLoop,
        _window_id: WindowId,
        event: WindowEvent,
    ) {
        match event {
            WindowEvent::CloseRequested => {
                event_loop.exit();
            }
            _ => (),
        }
    }

    fn exiting(&mut self, _: &ActiveEventLoop) {
        log::info!("Closing SURGE window");
        md::on_unload();
    }

    fn new_events(&mut self, _: &ActiveEventLoop, _: winit::event::StartCause) {
        self.loop_timer = Instant::now();
    }

    fn about_to_wait(&mut self, _: &ActiveEventLoop) {
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
        let desired_loop_time = 1.0 / (self.engine_config.renderer.fps_cap as f64);
        let loop_time = self.loop_timer.elapsed().as_secs_f64();

        log::info!("{}, {}", loop_time, 1.0 / loop_time);

        if self.engine_config.renderer.cap_fps && (loop_time < desired_loop_time) {
            std::thread::sleep(Duration::from_secs_f64(desired_loop_time - loop_time));
        }
    }
}

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

    /*******************
     * Init event loop *
     *******************/
    let event_loop = EventLoop::new().unwrap();
    event_loop.set_control_flow(ControlFlow::Poll);

    /******************
     * Engine context *
     ******************/
    let mut ctx = SurgeContext {
        window: None,
        vulkan_context: None,
        engine_config,
        loop_timer: Instant::now(),
    };

    /*************
     * Main Loop *
     *************/
    event_loop.run_app(&mut ctx).unwrap();
}
