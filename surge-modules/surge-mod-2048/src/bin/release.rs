use std::process::ExitCode;

fn main() -> ExitCode {
    let config = match surge_sdk::config::parse_config("config.toml") {
        Ok(config) => config,
        Err(e) => return surge_player::fail(e),
    };
    surge_player::run(surge_mod_2048::surge_register_app(), config)
}
