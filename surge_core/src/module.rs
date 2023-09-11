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

pub struct Module {
    library: dlopen::wrapper::Container<ModuleAPI>,
    name: String,
}

#[derive(Debug)]
pub enum ModuleError {
    LoadError,
    ReloadError,
    OnLoadError,
    OnUnloadError,
    UpdateError,
}

impl Module {
    pub fn load(module_base_name: &str) -> Result<Module, ModuleError> {
        log_info!("Loading module {}", module_base_name);

        unsafe {
            let library = match dlopen::wrapper::Container::load(module_base_name) {
                Ok(c) => c,
                Err(e) => {
                    log_error!("Unable to load module {}: {}", module_base_name, e);
                    return Err(ModuleError::LoadError);
                }
            };

            log_info!("Loaded {}", module_base_name);

            return Ok(Module {
                library,
                name: String::from(module_base_name),
            });
        }
    }

    pub fn load_from_config(config_file: &Vec<yaml_rust::Yaml>) -> Result<Module, ModuleError> {
        let module_base_name = opt_or_error!(
            config_file[0]["startup"]["module_name"].as_str(),
            ModuleError::LoadError,
            "Unable to recover the module file from the config file"
        );

        return Module::load(
            dlopen::utils::platform_file_name(module_base_name)
                .to_str()
                .unwrap(),
        );
    }

    pub fn unload(self) -> Result<u32, ModuleError> {
        self.checked_on_unload()?;
        drop(self);
        return Ok(0);
    }

    pub fn checked_on_load(&self) -> Result<u32, ModuleError> {
        unsafe {
            let result = self.library.on_load();
            if result != 0 {
                return Err(ModuleError::OnLoadError);
            } else {
                return Ok(result);
            }
        }
    }

    pub fn checked_on_unload(&self) -> Result<u32, ModuleError> {
        unsafe {
            let result = self.library.on_unload();
            if result != 0 {
                return Err(ModuleError::OnUnloadError);
            } else {
                return Ok(result);
            }
        }
    }

    pub fn update(&self, dt: f64) -> u32 {
        unsafe {
            return self.library.update(dt);
        }
    }

    pub fn checked_update(&self, dt: f64) -> Result<u32, ModuleError> {
        unsafe {
            let result = self.library.update(dt);
            if result != 0 {
                return Err(ModuleError::UpdateError);
            } else {
                return Ok(result);
            }
        }
    }
}

pub fn reload(module: Module) -> Result<Module, ModuleError> {
    let timer = std::time::Instant::now();

    log_info!("Hot reloading {:?}", module.name);

    let module_name = module.name.clone();

    let mut module_name_new = module.name.clone();
    module_name_new.push_str(".new");
    let module_name_new_path = std::path::Path::new(&module_name_new);

    module.unload()?;

    if module_name_new_path.exists() {
        log_info!(
            "{:?} found. Updating currently loaded module",
            module_name_new_path
        );
        let module_name_path = std::path::Path::new(&module_name);

        match std::fs::copy(module_name_new_path, module_name_path) {
            Err(e) => {
                log_info!("Unable to replace module with .new: {}", e);
                return Err(ModuleError::ReloadError);
            }
            _ => (),
        }
    } else {
        log_info!("No .new module version found. Reloading currently loaded module");
    }

    let new_module = Module::load(&module_name)?;

    log_info!(
        "Module {:?} hot reloaded in {} s",
        module_name,
        timer.elapsed().as_secs_f64()
    );

    new_module.checked_on_load()?;
    return Ok(new_module);
}
