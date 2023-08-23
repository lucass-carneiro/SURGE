#![feature(thread_id_value)]

#[macro_use]
mod logging;

fn main() {
    log_info!("This is an information");
    log_info!("This is an information with a number: {}", 1);
}
