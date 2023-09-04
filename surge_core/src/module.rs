use std::ffi::OsString;

use crate::log_error;
use crate::log_info;
use crate::opt_or_error;

#[derive(Debug)]
pub struct Module {
    lib: libloading::Library,
    name: OsString,
}

#[derive(Debug)]
pub enum ModuleError {
    LoadError,
    UnloadError,
    OnLoadCallError,
}

pub fn load(module_base_name: &str) -> Result<Module, ModuleError> {
    log_info!("Loading module {}", module_base_name);
    let module_name = libloading::library_filename(module_base_name);

    unsafe {
        let lib = match libloading::Library::new(&module_name) {
            Ok(l) => l,
            Err(e) => {
                log_error!("Unable to load module {}: {}", module_base_name, e);
                return Err(ModuleError::LoadError);
            }
        };

        let m = Module {
            lib,
            name: module_name,
        };

        log_info!("Loaded {}. Module object: {:?}", module_base_name, m);

        return Ok(m);
    }
}

pub fn load_from_config(config_file: &Vec<yaml_rust::Yaml>) -> Result<Module, ModuleError> {
    let module_base_name = opt_or_error!(
        config_file[0]["startup"]["module_name"].as_str(),
        ModuleError::LoadError,
        "Unable to recover the module file from the config file"
    );

    return load(module_base_name);
}

pub fn unload(module: Module) -> Result<(), ModuleError> {
    log_info!("Unloading module handle {:?}", module);

    on_unload(&module)?;

    match module.lib.close() {
        Ok(_) => Ok(()),
        Err(e) => {
            log_error!("Error unloading module: {}", e);
            return Err(ModuleError::UnloadError);
        }
    }
}

pub fn reload(module: &Module) -> Result<(), ModuleError> {
    log_info!("Reloading module {:?}", module.name);

    // Copy name before closing
    let module_name = module.name.clone();

    // Create the name with .new appended
    let mut module_name_new = module.name.clone();
    module_name_new.push(".new");
    let module_name_new_path = std::path::PathBuf::from(module_name_new);

    // Unload
    //on_unload(&module)?;
    //unload(module)?;

    if module_name_new_path.exists() {
        log_info!("Replace {:?} with {:?}", module_name_new_path, module_name);
    } else {
        log_info!(
            "No {:?} found. reloading module without updating",
            module_name_new_path
        );
    }

    // Load
    let new_handle = load(module_name.as_os_str());

    return Ok(());
}

pub type OnLoadFunc<'a> = libloading::Symbol<'a, unsafe extern "C" fn() -> u32>;
pub type OnUnloadFunc<'a> = libloading::Symbol<'a, unsafe extern "C" fn() -> u32>;
pub type UpdateFunc<'a> = libloading::Symbol<'a, unsafe extern "C" fn(f64) -> u32>;
pub type GetNameFunc<'a> = libloading::Symbol<'a, unsafe extern "C" fn() -> *const u8>;

pub fn on_load(module: &Module) -> Result<u32, ModuleError> {
    unsafe {
        match module.lib.get(b"on_load") {
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

pub fn on_unload(module: &Module) -> Result<u32, ModuleError> {
    unsafe {
        match module.lib.get(b"on_unload") {
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

pub fn get_update_handle(module: &Module) -> Result<UpdateFunc, ModuleError> {
    unsafe {
        match module.lib.get(b"update") {
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
