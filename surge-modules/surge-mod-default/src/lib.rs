extern crate surge_core;

use log;

#[unsafe(no_mangle)]
pub fn mod_func() {
    log::info!("This is a mod func!");
    surge_core::core_func();
}
