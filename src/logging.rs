#[cfg(feature = "log_color")]
macro_rules! log_info {
    ($msg:literal) => {
        println!(
            "\x1b[94m[{}]\x1b[m \x1b[36m[Thread ID: {}]\x1b[m \x1b[1m\x1b[32mSURGE Info:\x1b[m {}",
            chrono::Local::now().format("%Y-%m-%d %H:%M:%S"),
            std::thread::current().id().as_u64(),
            $msg
        )
    };

    ($fmt_string:tt, $($args:tt)*) => {
        println!(
            concat!(
                "\x1b[94m[{}]\x1b[m \x1b[36m[Thread ID: {}]\x1b[m \x1b[1m\x1b[32mSURGE Info:\x1b[m ",
                $fmt_string
            ),
            chrono::Local::now().format("%Y-%m-%d %H:%M:%S"),
            std::thread::current().id().as_u64(),
            $($args)*
        )
    };
}
