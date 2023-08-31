use crate::log_error;
use crate::log_info;
use crate::value_or_error;

#[derive(Debug)]
pub enum ShaderType {
    FragmentShader,
    VertexShader,
}

#[derive(Debug)]
pub enum ShaderError {
    ShaderFileNotFound,
    ShaderCompilationError,
    ShaderLinkingError,
}

fn load_and_compile_shader(path: &str, shader_type: ShaderType) -> Result<u32, ShaderError> {
    use std::ffi::CString;
    use std::fs;
    use std::ptr::null;
    use std::ptr::null_mut;
    use std::str;

    log_info!("Loading shader file {}", path);

    let file_string = value_or_error!(
        fs::read_to_string(path),
        ShaderError::ShaderFileNotFound,
        "Unable to load shader file"
    );

    let file_bytes = value_or_error!(
        CString::new(file_string.as_bytes()),
        ShaderError::ShaderFileNotFound,
        "Unable to convert file data to bytes"
    );

    unsafe {
        let shader_handle = match shader_type {
            ShaderType::VertexShader => gl::CreateShader(gl::VERTEX_SHADER),
            ShaderType::FragmentShader => gl::CreateShader(gl::FRAGMENT_SHADER),
        };

        gl::ShaderSource(shader_handle, 1, &file_bytes.as_ptr(), null());
        gl::CompileShader(shader_handle);

        let mut success = gl::FALSE as gl::types::GLint;
        let mut err_msg_buffer = Vec::with_capacity(1024);
        err_msg_buffer.set_len(1024 - 1); // subtract 1 to skip the trailing null character

        gl::GetShaderiv(shader_handle, gl::COMPILE_STATUS, &mut success);
        if success != gl::TRUE as gl::types::GLint {
            gl::GetShaderInfoLog(
                shader_handle,
                1024,
                null_mut(),
                err_msg_buffer.as_mut_ptr() as *mut gl::types::GLchar,
            );

            // We don't need the shader anymore.
            gl::DeleteShader(shader_handle);

            log_error!(
                "Shader {} handle {} compilation failed:\n  {}",
                path,
                shader_handle,
                str::from_utf8(&err_msg_buffer).unwrap()
            );
            return Err(ShaderError::ShaderCompilationError);
        } else {
            log_info!(
                "Shader {} handle {} compilation sucesfull",
                path,
                shader_handle
            );
            return Ok(shader_handle);
        }
    }
}

fn link_program(
    vertex_shader: u32,
    fragment_shader: u32,
    destroy_shaders: bool,
) -> Result<u32, ShaderError> {
    use std::ptr::null_mut;
    use std::str;

    unsafe {
        let program_handle = gl::CreateProgram();
        gl::AttachShader(program_handle, vertex_shader);
        gl::AttachShader(program_handle, fragment_shader);

        gl::LinkProgram(program_handle);

        let mut success = gl::FALSE as gl::types::GLint;
        let mut err_msg_buffer = Vec::with_capacity(1024);
        err_msg_buffer.set_len(1024 - 1); // subtract 1 to skip the trailing null character

        gl::GetProgramiv(program_handle, gl::LINK_STATUS, &mut success);
        if success != gl::TRUE as gl::types::GLint {
            gl::GetProgramInfoLog(
                program_handle,
                1024,
                null_mut(),
                err_msg_buffer.as_mut_ptr() as *mut gl::types::GLchar,
            );

            gl::DetachShader(program_handle, vertex_shader);
            gl::DetachShader(program_handle, fragment_shader);
            gl::DeleteProgram(program_handle);

            log_error!(
                "Failed to link shader handles {} and {} to create program {}:\n  {}",
                vertex_shader,
                fragment_shader,
                program_handle,
                str::from_utf8(&err_msg_buffer).unwrap()
            );

            return Err(ShaderError::ShaderLinkingError);
        } else {
            gl::DetachShader(program_handle, vertex_shader);
            gl::DetachShader(program_handle, fragment_shader);

            if destroy_shaders {
                log_info!(
                    "Destroying shader handles {} and {}",
                    vertex_shader,
                    fragment_shader
                );

                gl::DeleteShader(vertex_shader);
                gl::DeleteShader(fragment_shader);
            }

            log_info!(
                "Shader handles {} and {} linked sucesfully. Program handle: {}",
                vertex_shader,
                fragment_shader,
                program_handle
            );
            return Ok(program_handle);
        }
    }
}

pub fn create_shader_program(
    vertex_shader_path: &str,
    fragment_shader_path: &str,
) -> Result<u32, ShaderError> {
    let vertex_shader_handle =
        load_and_compile_shader(vertex_shader_path, ShaderType::VertexShader)?;

    let fragment_shader_handle =
        load_and_compile_shader(fragment_shader_path, ShaderType::FragmentShader)?;

    let linked_program = link_program(vertex_shader_handle, fragment_shader_handle, true)?;

    return Ok(linked_program);
}
