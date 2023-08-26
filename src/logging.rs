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

#[cfg(feature = "log_color")]
macro_rules! log_warning {
    ($msg:literal) => {
        println!(
            "\x1b[94m[{}]\x1b[m \x1b[36m[Thread ID: {}]\x1b[m \x1b[1m\x1b[33mSURGE Warning:\x1b[m {}",
            chrono::Local::now().format("%Y-%m-%d %H:%M:%S"),
            std::thread::current().id().as_u64(),
            $msg
        )
    };

    ($fmt_string:tt, $($args:tt)*) => {
        println!(
            concat!(
                "\x1b[94m[{}]\x1b[m \x1b[36m[Thread ID: {}]\x1b[m \x1b[1m\x1b[33mSURGE Warning:\x1b[m ",
                $fmt_string
            ),
            chrono::Local::now().format("%Y-%m-%d %H:%M:%S"),
            std::thread::current().id().as_u64(),
            $($args)*
        )
    };
}

#[cfg(feature = "log_color")]
macro_rules! log_error {
    ($msg:literal) => {
        println!(
            "\x1b[94m[{}]\x1b[m \x1b[36m[Thread ID: {}]\x1b[m \x1b[1m\x1b[31mSURGE Error:\x1b[m {}",
            chrono::Local::now().format("%Y-%m-%d %H:%M:%S"),
            std::thread::current().id().as_u64(),
            $msg
        )
    };

    ($fmt_string:tt, $($args:tt)*) => {
        println!(
            concat!(
                "\x1b[94m[{}]\x1b[m \x1b[36m[Thread ID: {}]\x1b[m \x1b[1m\x1b[31mSURGE Error:\x1b[m ",
                $fmt_string
            ),
            chrono::Local::now().format("%Y-%m-%d %H:%M:%S"),
            std::thread::current().id().as_u64(),
            $($args)*
        )
    };
}

#[cfg(not(feature = "log_color"))]
macro_rules! log_info {
    ($msg:literal) => {
        println!(
            "[{}] [Thread ID: {}] SURGE Info: {}",
            chrono::Local::now().format("%Y-%m-%d %H:%M:%S"),
            std::thread::current().id().as_u64(),
            $msg
        )
    };

    ($fmt_string:tt, $($args:tt)*) => {
        println!(
            concat!(
                "[{}] [Thread ID: {}] SURGE Info: ",
                $fmt_string
            ),
            chrono::Local::now().format("%Y-%m-%d %H:%M:%S"),
            std::thread::current().id().as_u64(),
            $($args)*
        )
    };
}

#[cfg(not(feature = "log_color"))]
macro_rules! log_warning {
    ($msg:literal) => {
        println!(
            "[{}] [Thread ID: {}] SURGE Warning: {}",
            chrono::Local::now().format("%Y-%m-%d %H:%M:%S"),
            std::thread::current().id().as_u64(),
            $msg
        )
    };

    ($fmt_string:tt, $($args:tt)*) => {
        println!(
            concat!(
                "[{}] [Thread ID: {}] SURGE Warning: ",
                $fmt_string
            ),
            chrono::Local::now().format("%Y-%m-%d %H:%M:%S"),
            std::thread::current().id().as_u64(),
            $($args)*
        )
    };
}

#[cfg(not(feature = "log_color"))]
macro_rules! log_error {
    ($msg:literal) => {
        println!(
            "[{}] \x1b[36m[Thread ID: {}] \x1b[31mSURGE Error: {}",
            chrono::Local::now().format("%Y-%m-%d %H:%M:%S"),
            std::thread::current().id().as_u64(),
            $msg
        )
    };

    ($fmt_string:tt, $($args:tt)*) => {
        println!(
            concat!(
                "[{}] \x1b[36m[Thread ID: {}] \x1b[31mSURGE Error: ",
                $fmt_string
            ),
            chrono::Local::now().format("%Y-%m-%d %H:%M:%S"),
            std::thread::current().id().as_u64(),
            $($args)*
        )
    };
}

macro_rules! value_or_error {
    ($exp:expr, $err_value:expr, $msg:tt) => {
        match $exp {
            Err(e) => {
                println!(
                    "\x1b[94m[{}]\x1b[m \x1b[36m[Thread ID: {}]\x1b[m \x1b[1m\x1b[31mSURGE Error:\x1b[m {}: {}",
                    chrono::Local::now().format("%Y-%m-%d %H:%M:%S"),
                    std::thread::current().id().as_u64(),
                    $msg,
                    e
                );
                return Err($err_value);
            },
            Ok(o) => o,
        }
    };
}
