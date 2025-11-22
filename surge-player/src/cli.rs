use std::io::Write;

pub(crate) fn init_env_logger() {
    use env_logger::Builder;
    use log::LevelFilter;

    let mut builder = Builder::from_default_env();
    builder.filter_level(LevelFilter::Trace);

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

pub(crate) fn print_logo() {
    let logo = r"    d888888o.   8 8888      88 8 888888888o.        ,o888888o.    8 8888888888
  .`8888:' `88. 8 8888      88 8 8888    `88.      8888     `88.  8 8888
  8.`8888.   Y8 8 8888      88 8 8888     `88   ,8 8888       `8. 8 8888
  `8.`8888.     8 8888      88 8 8888     ,88   88 8888           8 8888
   `8.`8888.    8 8888      88 8 8888.   ,88'   88 8888           8 888888888888
    `8.`8888.   8 8888      88 8 888888888P'    88 8888           8 8888
     `8.`8888.  8 8888      88 8 8888`8b        88 8888   8888888 8 8888
 8b   `8.`8888. ` 8888     ,8P 8 8888 `8b.      `8 8888       .8' 8 8888
 `8b.  ;8.`8888   8888   ,d8P  8 8888   `8b.       8888     ,88'  8 8888
  `Y8888P ,88P'    `Y88888P'   8 8888     `88.      `8888888P'    8 888888888888";
    println!("\x1b[1;38;2;220;20;60m{logo}\x1b[m");
}
