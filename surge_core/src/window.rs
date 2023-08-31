use crate::log_error;
use crate::log_info;
use crate::opt_or_error;
use crate::renderer;
use crate::value_or_error;

use glfw::Context;

fn glfw_error_callback(error_code: glfw::Error, description: String, _: &()) {
    log_error!("GLFW error code{}: {}", error_code, description);
}

#[derive(Debug)]
pub enum WindowError {
    InitError,
    WindowCreationError,
    WindowNameNotRecoverable,
    WindowCursorNotRecoverable,
    WindowWidthNotRecoverable,
    WindowHeightNotRecoverable,
    WindowVSyncNotRecoverable,
    WindowAVSyncNotRecoverable,
}

pub fn init(
    config_file: &Vec<yaml_rust::Yaml>,
) -> Result<
    (
        glfw::Glfw,
        glfw::Window,
        std::sync::mpsc::Receiver<(f64, glfw::WindowEvent)>,
    ),
    WindowError,
> {
    /*************
     * GLFW init *
     *************/
    log_info!("Initializing GLFW");

    // Create GLFW error callback. Register on init
    let error_callback = Some(glfw::Callback {
        f: glfw_error_callback as fn(glfw::Error, String, &()),
        data: (),
    });

    let mut glfw_ctx = value_or_error!(
        glfw::init(error_callback),
        WindowError::InitError,
        "Unable to initialize GLFW"
    );

    /************************
     * Config data recovery *
     **************************/
    log_info!("Recovering config data");

    let name = opt_or_error!(
        config_file[0]["window"]["name"].as_str(),
        WindowError::WindowNameNotRecoverable,
        "Unable to find window name field"
    );

    let cursor = opt_or_error!(
        config_file[0]["window"]["cursor"].as_bool(),
        WindowError::WindowCursorNotRecoverable,
        "Unable to find window cursor field"
    );

    let width = opt_or_error!(
        config_file[0]["window"]["resolution"]["width"].as_i64(),
        WindowError::WindowWidthNotRecoverable,
        "Unable to find window width field"
    );

    let height = opt_or_error!(
        config_file[0]["window"]["resolution"]["height"].as_i64(),
        WindowError::WindowHeightNotRecoverable,
        "Unable to find window height field"
    );

    let vsync = opt_or_error!(
        config_file[0]["window"]["VSync"]["enabled"].as_bool(),
        WindowError::WindowVSyncNotRecoverable,
        "Unable to find VSync enabled field"
    );

    let avsync = opt_or_error!(
        config_file[0]["window"]["VSync"]["adaptive"].as_bool(),
        WindowError::WindowAVSyncNotRecoverable,
        "Unable to find VSync adaptive field"
    );

    /****************
     * Window hints *
     *****************/
    log_info!("Setting window hints");
    glfw_ctx.window_hint(glfw::WindowHint::ContextVersion(4, 1));
    glfw_ctx.window_hint(glfw::WindowHint::OpenGlProfile(
        glfw::OpenGlProfileHint::Core,
    ));
    glfw_ctx.window_hint(glfw::WindowHint::Resizable(false));

    #[cfg(target_os = "macos")]
    glfw_ctx.window_hint(glfw::WindowHint::OpenGlForwardCompat(true));

    /****************************
     * Window init              *
     * TODO: Handle full screen *
     ****************************/
    log_info!("Initializing engine window");
    let (mut window, event_recv) = opt_or_error!(
        glfw_ctx.create_window(
            width as u32,
            height as u32,
            name,
            glfw::WindowMode::Windowed
        ),
        WindowError::WindowCreationError,
        "Unable to initialize GLFW window"
    );

    log_info!("Engine window created");

    /*******************************
     *           CURSORS           *
     *******************************/
    log_info!("Setting cursur mode");
    window.set_cursor_mode(if cursor {
        glfw::CursorMode::Normal
    } else {
        glfw::CursorMode::Normal
    });

    /***********************
     * OpenGL context init *
     ***********************/
    log_info!("Initializing OpenGL");

    window.make_current();
    window.set_key_polling(true);
    window.set_framebuffer_size_polling(true);

    if vsync {
        glfw_ctx.set_swap_interval(glfw::SwapInterval::Sync(1));
    } else if vsync && avsync {
        glfw_ctx.set_swap_interval(glfw::SwapInterval::Adaptive);
    } else {
        glfw_ctx.set_swap_interval(glfw::SwapInterval::None);
    }

    gl::load_with(|symbol| window.get_proc_address(symbol) as *const _);

    renderer::enable_capability(renderer::RendererCapability::DepthTesting);
    renderer::enable_capability(renderer::RendererCapability::AlphaBlending);

    return Ok((glfw_ctx, window, event_recv));
}
