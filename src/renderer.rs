#[derive(Debug)]
pub enum RendererError {
    ClearColorRedRecoveryError,
    ClearColorGreenRecoveryError,
    ClearColorBlueRecoveryError,
    ClearColorAlphaRecoveryError,
}

pub fn get_clear_color(
    config_file: &Vec<yaml_rust::Yaml>,
) -> Result<(f64, f64, f64, f64), RendererError> {
    let r = opt_or_error!(
        config_file[0]["renderer"]["clear_color"]["r"].as_f64(),
        RendererError::ClearColorRedRecoveryError,
        "Unable to recover renderer clear color red component"
    );

    let g = opt_or_error!(
        config_file[0]["renderer"]["clear_color"]["g"].as_f64(),
        RendererError::ClearColorGreenRecoveryError,
        "Unable to recover renderer clear color green component"
    );

    let b = opt_or_error!(
        config_file[0]["renderer"]["clear_color"]["b"].as_f64(),
        RendererError::ClearColorBlueRecoveryError,
        "Unable to recover renderer clear color blue component"
    );

    let a = opt_or_error!(
        config_file[0]["renderer"]["clear_color"]["a"].as_f64(),
        RendererError::ClearColorAlphaRecoveryError,
        "Unable to recover renderer clear color alpha component"
    );

    return Ok((r, g, b, a));
}

pub fn clear(clear_color: &(f64, f64, f64, f64)) {
    unsafe {
        gl::ClearColor(
            clear_color.0 as gl::types::GLfloat,
            clear_color.1 as gl::types::GLfloat,
            clear_color.2 as gl::types::GLfloat,
            clear_color.3 as gl::types::GLfloat,
        );
        gl::Clear(gl::COLOR_BUFFER_BIT);
        gl::Clear(gl::DEPTH_BUFFER_BIT);
    }
}

#[derive(Debug)]
pub enum RendererCapability {
    DepthTesting,
    AlphaBlending,
}

pub fn enable_capability(cap: RendererCapability) {
    match cap {
        RendererCapability::AlphaBlending => unsafe {
            gl::Enable(gl::DEPTH_TEST);
        },
        RendererCapability::DepthTesting => unsafe {
            gl::Enable(gl::BLEND);
            gl::BlendFunc(gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA)
        },
    }
}
