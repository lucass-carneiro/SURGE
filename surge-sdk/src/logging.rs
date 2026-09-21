use std::io::Write;

pub fn init_env_logger() {
    use env_logger::{Builder, Env};

    // `debug` is only the default: an explicit `RUST_LOG` wins.
    let mut builder = Builder::from_env(Env::default().default_filter_or("debug"));

    builder.format(|buf, record| {
        let banner = match record.level() {
            log::Level::Error => "\x1b[1m\x1b[31mSURGE Error\x1b[m",
            log::Level::Warn => "\x1b[1m\x1b[33mSURGE Warning\x1b[m",
            log::Level::Info => "\x1b[1m\x1b[32mSURGE Info\x1b[m",
            log::Level::Debug => "\x1b[1m\x1b[34mSURGE Debug\x1b[m",
            log::Level::Trace => "\x1b[1m\x1b[34mSURGE Trace\x1b[m",
        };

        writeln!(
            buf,
            "\x1b[36m[{}]\x1b[m {}: {}",
            record.module_path().unwrap_or("unknown"),
            banner,
            record.args()
        )
    });

    builder.init();
}
