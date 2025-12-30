use sc::vulkan::{VulkanContext, sprite_database as spd};
use std::{
    cell::RefCell,
    sync::Arc,
    time::{Duration, Instant},
};
use surge_core::{self as sc, module::SurgeModule};
use winit::{
    application::ApplicationHandler,
    dpi::PhysicalSize,
    event::WindowEvent,
    event_loop::{ActiveEventLoop, ControlFlow, EventLoop},
    window::{Fullscreen, Window, WindowId},
};

use surge_mod_default as md;

mod cli;

struct SurgeContext<ModuleType: SurgeModule> {
    window: Option<Arc<Window>>,
    vulkan_context: Option<Arc<RefCell<VulkanContext>>>,
    sprite_database: Option<spd::SpriteDatabase>,
    engine_config: sc::config::EngineConfig,
    module: ModuleType,
    frame_timer: Instant,
    pause_rendering: bool,
}

impl<ModuleType> ApplicationHandler for SurgeContext<ModuleType>
where
    ModuleType: SurgeModule,
{
    /// Game loop start
    fn new_events(&mut self, _: &ActiveEventLoop, _: winit::event::StartCause) {
        self.frame_timer = Instant::now();
    }

    /// Game loop body
    fn about_to_wait(&mut self, _: &ActiveEventLoop) {
        // Stop rendering if minimized
        if self.pause_rendering {
            return;
        }

        // Handle hot reloading
        // Call module update
        self.module.update();

        // Acquire swapchain image
        let mut swpc_img_data = self
            .vulkan_context
            .as_mut()
            .unwrap()
            .borrow_mut()
            .request_swpc_img(&self.engine_config)
            .unwrap();

        if swpc_img_data.suboptimal {
            log::info!("Frame skipped due to suboptimal swapchain");
            return;
        }

        // Begin command recording
        self.vulkan_context
            .as_ref()
            .unwrap()
            .borrow()
            .cmd_begin(swpc_img_data.index)
            .unwrap();

        // Begin rendering
        self.vulkan_context
            .as_ref()
            .unwrap()
            .borrow()
            .cmd_render_begin(swpc_img_data.index, &self.engine_config);

        // Call module draw
        self.module.draw();

        // End rendering
        self.vulkan_context
            .as_ref()
            .unwrap()
            .borrow()
            .cmd_render_end();

        // End command recording
        self.vulkan_context
            .as_ref()
            .unwrap()
            .borrow()
            .cmd_end(swpc_img_data.index)
            .unwrap();

        // Submit command buffer
        self.vulkan_context
            .as_ref()
            .unwrap()
            .borrow()
            .cmd_submit()
            .unwrap();

        // Present
        self.vulkan_context
            .as_ref()
            .unwrap()
            .borrow_mut()
            .present_swpc(&mut swpc_img_data, &self.engine_config)
            .unwrap();

        // Refresh HR key state

        // FPS Cap.
        let desired_frame_time = 1.0 / (self.engine_config.renderer.fps_cap as f64);
        let frame_time = self.frame_timer.elapsed().as_secs_f64();

        if !self.engine_config.renderer.vsync
            && self.engine_config.renderer.cap_fps
            && (frame_time < desired_frame_time)
        {
            std::thread::sleep(Duration::from_secs_f64(desired_frame_time - frame_time));
        }
    }

    /// Game loop startup
    fn resumed(&mut self, event_loop: &ActiveEventLoop) {
        // Init window
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
                    // Init Vulkan
                    let w = Arc::new(window);
                    self.vulkan_context = Some(Arc::new(RefCell::new(
                        sc::vulkan::VulkanContext::new(&event_loop, &w, &self.engine_config)
                            .unwrap(),
                    )));

                    // Init sprite database
                    self.sprite_database = Some(
                        spd::SpriteDatabase::new(
                            self.vulkan_context.as_ref().unwrap().clone(),
                            spd::CreateInfo::default(),
                        )
                        .unwrap(),
                    );

                    //Save window to context
                    self.window = Some(w);

                    // Load first module
                    self.module.on_load();
                }
                Err(e) => {
                    log::error!("Unable to create SURGE window: {}", e);
                    event_loop.exit();
                }
            }
        }
    }

    /// Game loop Shutdown
    fn exiting(&mut self, _: &ActiveEventLoop) {
        log::info!("Closing SURGE window");
        self.module.on_unload();

        // We need to destroy the swapchain here because Winnit
        // drops the surface before we have a chance to drop the Vulkan context.
        // We can't have this, so we must destroy the swapchain before Winnit drops
        // the window surface.
        self.vulkan_context
            .as_ref()
            .unwrap()
            .borrow_mut()
            .destroy_swapchain()
            .unwrap();
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
            WindowEvent::Occluded(o) => {
                self.pause_rendering = o;
            }
            WindowEvent::Resized(size) => {
                if size.width == 0 && size.height == 0 {
                    self.pause_rendering = true;
                }
            }
            WindowEvent::KeyboardInput {
                device_id,
                event,
                is_synthetic,
            } => {
                self.module.keyboard_event(device_id, event, is_synthetic);
            }
            WindowEvent::MouseInput {
                device_id,
                state,
                button,
            } => {
                self.module.mouse_button_event(device_id, state, button);
            }
            WindowEvent::MouseWheel {
                device_id,
                delta,
                phase,
            } => {
                self.module.mouse_wheel_event(device_id, delta, phase);
            }
            _ => (),
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
        sprite_database: None,
        engine_config,
        module: md::ModuleDefault::new(),
        frame_timer: Instant::now(),
        pause_rendering: false,
    };

    /*************
     * Main Loop *
     *************/
    event_loop.run_app(&mut ctx).unwrap();
}
