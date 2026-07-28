use crate::errors::AppError;
use crate::vulkan::sprite_database::SpriteDatabase;
use libloading::{Library, Symbol};
use std::ops::{Deref, DerefMut};
use std::path::Path;
use winit::event::{DeviceId, ElementState, KeyEvent, MouseButton, MouseScrollDelta, TouchPhase};

/// A loaded SURGE application with its associated dynamic library
///
/// IMPORTANT: Field order matters! `app` must be dropped before `_library`
/// to ensure the trait object is destroyed before the library is unloaded.
pub struct LoadedApp {
    app: Box<dyn SurgeApp>,
    _library: Library, // Keep alive; underscore prefix to indicate intentionally unused
}

impl Deref for LoadedApp {
    type Target = dyn SurgeApp;

    fn deref(&self) -> &Self::Target {
        &*self.app
    }
}

impl DerefMut for LoadedApp {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut *self.app
    }
}

/// Defines a SURGE application
pub trait SurgeApp {
    /// Called when the application starts
    fn on_load(&mut self, _: &mut SpriteDatabase) {}

    /// Called if the swapchain is recreated
    fn on_swapchain_recreate(&mut self, _: &mut SpriteDatabase) {}

    /// Update application state
    fn update(&mut self, _: f32, _: &mut SpriteDatabase) {}

    /// Called when the application recieves a keyboard event
    fn keyboard_event(&mut self, _: DeviceId, _: KeyEvent, _: bool) {}

    /// Called when the application recieves a mouse button event
    fn mouse_button_event(&mut self, _: DeviceId, _: ElementState, _: MouseButton) {}

    /// Called when the application recieves a mouse wheel event
    fn mouse_wheel_event(&mut self, _: DeviceId, _: MouseScrollDelta, _: TouchPhase) {}

    /// Called when engin is about to exit, and the application is about to quit
    fn on_unload(&mut self) {}
}

pub fn load_from_dylib(dylib_folder: &str, dylib_name: &str) -> Result<LoadedApp, AppError> {
    let app_path_string = {
        #[cfg(target_os = "windows")]
        {
            format!("{}\\{}.dll", dylib_folder, dylib_name)
        }
        #[cfg(target_os = "linux")]
        {
            format!("{}/lib{}.so", dylib_folder, dylib_name)
        }
    };

    let app_path = Path::new(&app_path_string)
        .canonicalize()
        .map_err(|io_error| AppError::IoError {
            name: dylib_name.to_string(),
            io_error,
        })?;

    let app_library = unsafe {
        Library::new(app_path.as_os_str()).map_err(|lib_error| AppError::LibLoadingError {
            name: dylib_name.to_string(),
            lib_error,
        })
    }?;

    let surge_register_app: Symbol<fn() -> Box<dyn SurgeApp>> = unsafe {
        app_library
            .get("surge_register_app")
            .map_err(|lib_error| AppError::LibLoadingError {
                name: dylib_name.to_string(),
                lib_error,
            })
    }?;

    let app = surge_register_app();

    Ok(LoadedApp {
        app,
        _library: app_library,
    })
}
