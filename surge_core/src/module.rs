use crate::log_error;
use crate::log_info;

#[derive(Debug)]
pub enum ModuleError {
    LoadError,
    UnloadError,
    OnLoadCallError,
}

pub fn load(module_name: &str) -> Result<libloading::Library, ModuleError> {
    log_info!("Loading module {}", module_name);

    unsafe {
        let lib = match libloading::Library::new(module_name) {
            Ok(l) => l,
            Err(e) => {
                log_error!("Unable to load module {}: {}", module_name, e);
                return Err(ModuleError::LoadError);
            }
        };

        log_info!("Loaded {}. Module object: {:?}", module_name, lib);
        return Ok(lib);
    }
}

pub fn unload(module: libloading::Library) -> Result<(), ModuleError> {
    log_info!("Unloading module {:?}", module);

    on_unload(&module)?;

    match module.close() {
        Ok(_) => Ok(()),
        Err(e) => {
            log_error!("Error loading library: {}", e);
            return Err(ModuleError::UnloadError);
        }
    }
}

pub type OnLoadFunc<'a> = libloading::Symbol<'a, unsafe extern "C" fn() -> u32>;
pub type OnUnloadFunc<'a> = libloading::Symbol<'a, unsafe extern "C" fn() -> u32>;
pub type UpdateFunc<'a> = libloading::Symbol<'a, unsafe extern "C" fn(f64) -> u32>;

pub fn on_load(module: &libloading::Library) -> Result<u32, ModuleError> {
    unsafe {
        match module.get(b"on_load") {
            Ok::<OnLoadFunc, _>(func) => {
                log_info!("Calling on_load on module {:?}", module);
                return Ok(func());
            }
            Err(e) => {
                log_error!("Unable to call on_load for module {:?}: {}", module, e);
                return Err(ModuleError::OnLoadCallError);
            }
        }
    }
}

pub fn on_unload(module: &libloading::Library) -> Result<u32, ModuleError> {
    unsafe {
        match module.get(b"on_unload") {
            Ok::<OnUnloadFunc, _>(func) => {
                log_info!("Calling on_unload on module {:?}", module);
                return Ok(func());
            }
            Err(e) => {
                log_error!("Unable to call on_unload for module {:?}: {}", module, e);
                return Err(ModuleError::OnLoadCallError);
            }
        }
    }
}

pub fn get_update_handle(module: &libloading::Library) -> Result<UpdateFunc, ModuleError> {
    unsafe {
        match module.get(b"update") {
            Ok::<UpdateFunc, _>(func) => {
                log_info!("Obtained handle for update on module {:?}", module);
                return Ok(func);
            }
            Err(e) => {
                log_error!(
                    "Unable to obtain handle to update for module {:?}: {}",
                    module,
                    e
                );
                return Err(ModuleError::OnLoadCallError);
            }
        }
    }
}
