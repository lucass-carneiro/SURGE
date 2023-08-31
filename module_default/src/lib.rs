#![feature(thread_id_value)]

use surge_core::chrono;
use surge_core::log_info;

#[no_mangle]
pub extern "C" fn on_load() -> u32 {
    log_info!("Loading default module");
    return 0;
}

#[no_mangle]
pub extern "C" fn on_unload() -> u32 {
    log_info!("Unloading default module");
    return 0;
}

#[no_mangle]
pub extern "C" fn update(_: f64) -> u32 {
    return 0;
}
