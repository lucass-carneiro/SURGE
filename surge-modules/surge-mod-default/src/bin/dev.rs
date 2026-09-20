fn main() {
    surge_sdk::init_env_logger();
    surge_sdk::run_dev_module(surge_mod_default::surge_register_app());
}
