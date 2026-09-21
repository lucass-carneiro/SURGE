use sc::vulkan::{VulkanContext, sprite_database as spd};
use std::{
    cell::RefCell,
    sync::Arc,
    time::{Duration, Instant},
};
use surge_core::{self as sc};
use surge_sdk::{SurgeApp, config::EngineConfig};
use winit::{
    application::ApplicationHandler,
    dpi::PhysicalSize,
    event::{ElementState, WindowEvent},
    event_loop::{ActiveEventLoop, ControlFlow, EventLoop},
    keyboard::{KeyCode, PhysicalKey},
    window::{Fullscreen, Window, WindowId},
};

struct SurgeContext {
    window: Option<Arc<Window>>,
    vulkan_context: Option<Arc<RefCell<VulkanContext>>>,
    sprite_database: Option<spd::SpriteDatabase>,
    engine_config: EngineConfig,
    startup_app: Box<dyn SurgeApp>,
    frame_timer: Instant,
    last_update_call_timer: Instant,
    pause_rendering: bool,
    // True while `about_to_wait` is sleeping (paused or faulted), so the first frame after
    // waking can discard the time spent asleep instead of handing it to `update` as `dt`.
    was_idle: bool,
    recreate_swapchain: bool,
    /// Whether the window title currently carries the crash suffix, so the transition into and
    /// out of the faulted state is acted on once rather than every frame.
    crash_shown: bool,
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
    fn about_to_wait(&mut self, event_loop: &ActiveEventLoop) {
        // A faulted module freezes the window on its last presented frame: nothing is acquired,
        // drawn or presented until a reload clears the fault.
        let faulted = self.startup_app.is_faulted();
        if faulted != self.crash_shown {
            self.crash_shown = faulted;
            let mut title = self.engine_config.window.name.clone();
            if faulted {
                log::error!("module crashed; frames are suspended until it is reloaded (F5)");
                title.push_str(" — MODULE CRASHED");
            }
            if let Some(window) = &self.window {
                window.set_title(&title);
            }
        }

        // While there is nothing to render, sleep until the next window event instead of
        // spinning: every way out of these states (F5, resize) arrives as an event.
        // A fault discovered inside `update()` below returns with `Poll` still set, so the
        // next tick still gets here to update the title before going to sleep.
        if faulted || self.pause_rendering {
            event_loop.set_control_flow(ControlFlow::Wait);
            self.was_idle = true;
            return;
        }
        event_loop.set_control_flow(ControlFlow::Poll);
        if self.was_idle {
            // Time spent paused or faulted is not simulation time.
            self.last_update_call_timer = Instant::now();
            self.was_idle = false;
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

        // Call startup app update
        let dt = self.last_update_call_timer.elapsed().as_secs_f32();
        self.startup_app
            .update(dt, self.sprite_database.as_mut().unwrap());
        self.last_update_call_timer = Instant::now();

        // A fault is only discovered inside `update()`, i.e. after the acquire above. Presenting
        // this frame would replace the last good image with an empty one, so drop it instead: the
        // acquired image, its semaphore and the reset fence are all discarded by the swapchain
        // recreate that the reload clearing this fault always schedules.
        if self.startup_app.is_faulted() {
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
                    if !self.pause_rendering {
                        log::info!("Pausing (window has no area)");
                        self.pause_rendering = true;
                    }
                    return;
                }

                // Any non-zero size is the way out of a minimize-pause. Focus is deliberately
                // not involved: a restore is not guaranteed to deliver `Focused(true)`, and a
                // window that never took focus must still render.
                if self.pause_rendering {
                    log::info!("Resuming");
                    self.pause_rendering = false;
                }

                if size != self.last_window_size {
                    // The surface's actual extent changed (e.g. compositor-imposed
                    // resize, DPI change), so the swapchain must be rebuilt. This does
                    // NOT change engine_config.resolution: that stays the fixed design
                    // resolution, and create_swapchain() clamps it to whatever the
                    // surface capabilities actually allow.
                    self.last_window_size = size;
                    self.recreate_swapchain = true;
                }
            }
            WindowEvent::KeyboardInput {
                device_id,
                event,
                is_synthetic,
            } => {
                // Throwaway dev reload key: handled here, never forwarded to the module.
                if event.physical_key == PhysicalKey::Code(KeyCode::F5) {
                    if event.state == ElementState::Pressed && !event.repeat {
                        log::info!("Reloading module");
                        self.recreate_swapchain |= self.startup_app.reload();
                    }
                    return;
                }

                // winit's KeyEvent is not constructible outside winit, so the module
                // interface takes the SDK's mirror of its public fields.
                let event = surge_sdk::KeyEvent {
                    physical_key: event.physical_key,
                    logical_key: event.logical_key,
                    text: event.text.map(|text| text.to_string()),
                    location: event.location,
                    state: event.state,
                    repeat: event.repeat,
                };
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

/// Runs `app` in a window configured by `config` until the window closes.
///
/// Owns process-wide startup (logo, logger), so it must be the first thing in the process to
/// touch `log`, and it may only be called once.
pub fn run(app: Box<dyn SurgeApp>, engine_config: EngineConfig) {
    /********
     * Logo *
     ********/
    sc::cli::print_logo();

    /*****************
     * Logger set-up *
     *****************/
    surge_sdk::init_env_logger();

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
        startup_app: app,
        frame_timer: Instant::now(),
        last_update_call_timer: Instant::now(),
        pause_rendering: false,
        was_idle: false,
        recreate_swapchain: false,
        crash_shown: false,
        last_window_size: initial_size,
    };

    /*************
     * Main Loop *
     *************/
    event_loop.run_app(&mut ctx).unwrap();
}
