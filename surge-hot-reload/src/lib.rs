extern crate surge_core;

use surge_core::errors::ModuleError;

use libloading::{Library, Symbol};

use std::path::Path;

/// A hot reloadable SURGE module
#[derive(Debug)]
pub struct HotReloadModule {
    module_name: String,
    module_library: Library,
}

impl HotReloadModule {
    /// Loads a module.
    ///
    /// Arguments:
    /// - `module_folder`: The folder that contains the module.
    /// - `module_name`: Name of the module (do not append extension or path)
    pub fn load_module(module_folder: &str, module_name: &str) -> Result<Self, ModuleError> {
        let module_path_string = {
            #[cfg(target_os = "windows")]
            {
                format!("{}/{}.so", module_folder, module_name)
            }
            #[cfg(target_os = "linux")]
            {
                format!("{}/lib{}.so", module_folder, module_name)
            }
        };

        let module_path = match Path::new(&module_path_string).canonicalize() {
            Ok(path) => path,
            Err(io_error) => {
                return Err(ModuleError::IoError {
                    name: module_name.to_string(),
                    io_error,
                });
            }
        };

        let module_library = match unsafe { Library::new(module_path.as_os_str()) } {
            Ok(lib) => lib,
            Err(e) => {
                return Err(ModuleError::LibLoadingError {
                    name: module_name.to_string(),
                    lib_error: e,
                });
            }
        };

        Ok(HotReloadModule {
            module_name: module_name.to_string(),
            module_library,
        })
    }

    pub fn load_symbol<Signature>(
        &self,
        symbol_name: &str,
    ) -> Result<Symbol<'_, Signature>, ModuleError> {
        match unsafe { self.module_library.get(symbol_name) } {
            Ok(lib) => Ok(lib),
            Err(e) => {
                return Err(ModuleError::LibLoadingError {
                    name: self.module_name.clone(),
                    lib_error: e,
                });
            }
        }
    }
}
