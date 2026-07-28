use sc::vulkan::{VulkanContext, sprite_database as spd};
use std::{
    cell::RefCell,
    sync::Arc,
    time::{Duration, Instant},
};
use surge_core::{self as sc};
use winit::{
    application::ApplicationHandler,
    dpi::PhysicalSize,
    event::WindowEvent,
    event_loop::{ActiveEventLoop, ControlFlow, EventLoop},
    window::{Fullscreen, Window, WindowId},
};

struct SurgeContext {
    window: Option<Arc<Window>>,
    vulkan_context: Option<Arc<RefCell<VulkanContext>>>,
    sprite_database: Option<spd::SpriteDatabase>,
    engine_config: sc::config::EngineConfig,
    startup_app: sc::app::LoadedApp,
    frame_timer: Instant,
    last_update_call_timer: Instant,
    pause_rendering: bool,
    recreate_swapchain: bool,
    // Tracks the actual observed window size, purely to dedup redundant Resized
    // events. This is NOT the engine's configured resolution: config.resolution
    // is a fixed design constant (depth image extent, ortho projection, module
    // pixel math) and must never be overwritten by the window manager.
    last_window_size: PhysicalSize<u32>,
}

impl ApplicationHandler for SurgeContext {
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

        // Reinitialize if necessary
        if self.recreate_swapchain {
            self.vulkan_context
                .as_mut()
                .unwrap()
                .borrow_mut()
                .recreate_swapchain(&self.engine_config)
                .unwrap();

            self.sprite_database = Some(
                spd::SpriteDatabase::new(
                    self.vulkan_context.as_ref().unwrap().clone(),
                    spd::CreateInfo {
                        blending_mode: spd::BlendingMode::Alpha,
                        texture_filtering_mode: spd::TextureFilteringMode::Linear,
                        texture_filtering_level: spd::TextureFilteringLevel::X4,
                        max_sprites: 32,
                        max_textures: 32,
                        window_width: self.engine_config.resolution.width as f32,
                        window_height: self.engine_config.resolution.height as f32,
                    },
                )
                .unwrap(),
            );

            self.startup_app
                .on_swapchain_recreate(self.sprite_database.as_mut().unwrap());

            self.recreate_swapchain = false;
        }

        // Acquire swapchain image. This must happen (and wait on the frame's fence)
        // before app.update(), since update() writes this frame's instance SSBO and
        // the fence is what guarantees the GPU is done reading that same buffer.
        let mut swpc_img_data = match self
            .vulkan_context
            .as_mut()
            .unwrap()
            .borrow_mut()
            .request_swpc_img()
            .unwrap()
        {
            Some(data) => data,
            None => {
                // Swapchain is out of date (e.g. resize, monitor/DPI change, compositor
                // restart) - there is no valid image to render into this tick.
                self.recreate_swapchain = true;
                return;
            }
        };

        self.recreate_swapchain |= swpc_img_data.suboptimal;

        // Handle hot reloading
        // Call startup app update
        let dt = self.last_update_call_timer.elapsed().as_secs_f32();
        self.startup_app
            .update(dt, self.sprite_database.as_mut().unwrap());
        self.last_update_call_timer = Instant::now();

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

        // Draw sprites
        self.sprite_database.as_mut().unwrap().draw().unwrap();

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
        let present_out_of_date = self
            .vulkan_context
            .as_ref()
            .unwrap()
            .borrow_mut()
            .present_swpc(&mut swpc_img_data)
            .unwrap();

        self.recreate_swapchain |= present_out_of_date || swpc_img_data.suboptimal;

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
                .with_resizable(false)
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
                            spd::CreateInfo {
                                blending_mode: spd::BlendingMode::Alpha,
                                texture_filtering_mode: spd::TextureFilteringMode::Linear,
                                texture_filtering_level: spd::TextureFilteringLevel::X4,
                                max_sprites: 32,
                                max_textures: 32,
                                window_width: self.engine_config.resolution.width as f32,
                                window_height: self.engine_config.resolution.height as f32,
                            },
                        )
                        .unwrap(),
                    );

                    //Save window to context
                    self.window = Some(w);

                    // Load startup app
                    self.startup_app
                        .on_load(self.sprite_database.as_mut().unwrap());
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
        self.startup_app.on_unload();

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
            WindowEvent::Resized(size) => {
                log::info!(
                    "Resizing window ({},{}) -> ({},{})",
                    self.last_window_size.width,
                    self.last_window_size.height,
                    size.width,
                    size.height
                );

                if size.width == 0 || size.height == 0 {
                    log::info!("Pausing");
                    self.pause_rendering = true;
                } else if size != self.last_window_size {
                    // The surface's actual extent changed (e.g. compositor-imposed
                    // resize, DPI change), so the swapchain must be rebuilt. This does
                    // NOT change engine_config.resolution: that stays the fixed design
                    // resolution, and create_swapchain() clamps it to whatever the
                    // surface capabilities actually allow.
                    self.last_window_size = size;
                    self.recreate_swapchain = true;
                }
            }
            WindowEvent::Focused(true) => {
                if self.pause_rendering {
                    log::info!("Resuming");
                    self.pause_rendering = false;
                }
            }
            WindowEvent::Focused(false) => {
                if !self.pause_rendering {
                    log::info!("Pausing");
                    self.pause_rendering = true;
                }
            }
            WindowEvent::KeyboardInput {
                device_id,
                event,
                is_synthetic,
            } => {
                self.startup_app
                    .keyboard_event(device_id, event, is_synthetic);
            }
            WindowEvent::MouseInput {
                device_id,
                state,
                button,
            } => {
                self.startup_app
                    .mouse_button_event(device_id, state, button);
            }
            WindowEvent::MouseWheel {
                device_id,
                delta,
                phase,
            } => {
                self.startup_app.mouse_wheel_event(device_id, delta, phase);
            }
            _ => (),
        }
    }
}

pub fn main() {
    /********
     * Logo *
     ********/
    sc::cli::print_logo();

    /*********************
     * Parse config file *
     *********************/
    sc::cli::init_env_logger();

    // Parse config
    let engine_config = sc::config::parse_config("config.toml").unwrap();

    // Load startup app library
    let startup_app = surge_core::app::load_from_dylib(
        &engine_config.startup_app.app_folder,
        &engine_config.startup_app.app_name,
    )
    .unwrap();

    /*******************
     * Init event loop *
     *******************/
    let event_loop = EventLoop::new().unwrap();
    event_loop.set_control_flow(ControlFlow::Poll);

    /******************
     * Engine context *
     ******************/
    let initial_size = PhysicalSize::new(
        engine_config.resolution.width,
        engine_config.resolution.height,
    );

    let mut ctx = SurgeContext {
        window: None,
        vulkan_context: None,
        sprite_database: None,
        engine_config,
        startup_app,
        frame_timer: Instant::now(),
        last_update_call_timer: Instant::now(),
        pause_rendering: false,
        recreate_swapchain: false,
        last_window_size: initial_size,
    };

    /*************
     * Main Loop *
     *************/
    event_loop.run_app(&mut ctx).unwrap();
}
