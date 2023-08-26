#![feature(thread_id_value)]

#[macro_use]
mod logging;
mod cli;

fn main() {
    cli::draw_logo();
    let config_file = cli::parse_cfg().unwrap();
    print!("{:?}", config_file)
}
