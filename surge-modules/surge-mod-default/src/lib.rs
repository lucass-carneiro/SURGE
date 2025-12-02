extern crate surge_core;

use log;

#[unsafe(no_mangle)]
pub fn on_load() {
    log::info!("Module on_load");
}

#[unsafe(no_mangle)]
pub fn on_unload() {
    log::info!("Module on_unload");
}

#[unsafe(no_mangle)]
pub fn update() {
    // TODO
}

#[unsafe(no_mangle)]
pub fn draw() {
    // Bind pipeline
    // Set viewport
    // Set scissor
    // Draw
}
