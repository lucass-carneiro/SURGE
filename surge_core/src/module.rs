use crate::log_error;
use crate::log_info;
use crate::opt_or_error;

use dlopen::wrapper::WrapperApi;
use dlopen_derive::WrapperApi;

#[derive(WrapperApi)]
pub struct ModuleAPI {
    on_load: unsafe extern "C" fn() -> u32,
    on_unload: unsafe extern "C" fn() -> u32,
    update: unsafe extern "C" fn(dt: f64) -> u32,
}

pub type Module = dlopen::wrapper::Container<ModuleAPI>;

#[derive(Debug)]
pub enum ModuleError {
    LoadError,
    OnLoadError,
    OnUnloadError,
    UpdateError,
}

pub fn load(module_base_name: &str) -> Result<(Module, std::ffi::OsString), ModuleError> {
    log_info!("Loading module {}", module_base_name);

    unsafe {
        let module = match dlopen::wrapper::Container::load(module_base_name) {
            Ok(c) => c,
            Err(e) => {
                log_error!("Unable to load module {}: {}", module_base_name, e);
                return Err(ModuleError::LoadError);
            }
        };

        log_info!("Loaded {}", module_base_name);

        return Ok((module, dlopen::utils::platform_file_name(module_base_name)));
    }
}

pub fn load_from_config(
    config_file: &Vec<yaml_rust::Yaml>,
) -> Result<(Module, std::ffi::OsString), ModuleError> {
    let module_base_name = opt_or_error!(
        config_file[0]["startup"]["module_name"].as_str(),
        ModuleError::LoadError,
        "Unable to recover the module file from the config file"
    );

    return load(module_base_name);
}

pub fn unload(module: Module) -> Result<u32, ModuleError> {
    checked_on_unload(&module)?;
    drop(module);
    return Ok(0);
}

pub fn reload(module_name: &std::ffi::OsString) {
    log_info!("Hot reloading {:?}", module_name);

    let mut module_name_new = module_name.clone();
    module_name_new.push(".new");

    let module_name_new_path = std::path::Path::new(&module_name);

    if module_name_new_path.exists() {
        log_info!("{:?} exists. Replacing current module", &module_name_new);
    } else {
        log_info!(
            "{:?} does not exist. Reloading currently loaded module",
            &module_name_new
        );
    }
}

pub fn checked_on_load(module: &Module) -> Result<u32, ModuleError> {
    unsafe {
        let result = module.on_load();
        if result != 0 {
            return Err(ModuleError::OnLoadError);
        } else {
            return Ok(result);
        }
    }
}

pub fn checked_on_unload(module: &Module) -> Result<u32, ModuleError> {
    unsafe {
        let result = module.on_unload();
        if result != 0 {
            return Err(ModuleError::OnUnloadError);
        } else {
            return Ok(result);
        }
    }
}

pub fn checked_update(module: &Module, dt: f64) -> Result<u32, ModuleError> {
    unsafe {
        let result = module.update(dt);
        if result != 0 {
            return Err(ModuleError::UpdateError);
        } else {
            return Ok(result);
        }
    }
}
