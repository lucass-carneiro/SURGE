fn main() {
    let config = surge_sdk::config::parse_config("config.toml").unwrap();
    surge_player::run(surge_mod_2048::surge_register_app(), config);
}
