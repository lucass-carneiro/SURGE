#include "opengl/program.hpp"

#include "allocator.hpp"
#include "file.hpp"
#include "logging_system/logging_system.hpp"

auto surge::load_and_compile(const char *shader_source, const char *shader_name,
                             GLenum shader_type) noexcept -> std::optional<GLuint> {

  log_info("Compiling shader {}", shader_name);

  // Create an empty shader handle
  const GLuint shader_handle_tmp = glCreateShader(shader_type);

  // Send the shader source code to GL
  glShaderSource(shader_handle_tmp, 1, &shader_source, nullptr);

  // Compile the shader
  glCompileShader(shader_handle_tmp);

  GLint is_compiled{0};
  glGetShaderiv(shader_handle_tmp, GL_COMPILE_STATUS, &is_compiled);

  if (is_compiled == GL_FALSE) {
    GLint max_length{0};
    glGetShaderiv(shader_handle_tmp, GL_INFO_LOG_LENGTH, &max_length);

    // The maxLength includes the NULL character
    GLsizei returnd_info_size{0};
    std::array<GLchar, SURGE_OPENGL_ERROR_BUFFER_SIZE> info_log{};

    glGetShaderInfoLog(shader_handle_tmp, SURGE_OPENGL_ERROR_BUFFER_SIZE, &returnd_info_size,
                       info_log.data());

    // We don't need the shader anymore.
    glDeleteShader(shader_handle_tmp);

    log_error("Shader \"{}\" compilation failed:\n  {}", shader_name, info_log.data());

    return {};
  } else {
    log_error("Shader \"{}\" compilation succesfull", shader_name);
    return shader_handle_tmp;
  }
}

auto surge::load_and_compile(const std::filesystem::path &p, GLenum shader_type) noexcept
    -> std::optional<GLuint> {

#ifdef SURGE_SYSTEM_Windows
  log_info(L"Loading shader file {}", p.c_str());
#else
  log_info("Loading shader file {}", p.c_str());
#endif

  load_file_return_t file{};

  if (shader_type == GL_VERTEX_SHADER) {
    file = load_file(p, ".vert", true);

  } else if (shader_type == GL_FRAGMENT_SHADER) {
    file = load_file(p, ".frag", true);

  } else {
    log_error("Unrecognized shader type {}", shader_type);
    return {};
  }

  if (!file) {
#ifdef SURGE_SYSTEM_Windows
    log_error(L"Unable to load shader file {}", p.c_str());
#else
    log_error("Unable to load shader file {}", p.c_str());
#endif
    return {};
  }

  // Get the source code in GL format
  auto file_source{static_cast<const GLchar *>(static_cast<void *>(file.value().data()))};

  // Create an empty shader handle
  const GLuint shader_handle_tmp = glCreateShader(shader_type);

  // Send the shader source code to GL
  glShaderSource(shader_handle_tmp, 1, &file_source, nullptr);

  // Compile the shader
  glCompileShader(shader_handle_tmp);

  // The file is no longer needed.
  mi_free(static_cast<void *>(file.value().data()));

  GLint is_compiled{0};
  glGetShaderiv(shader_handle_tmp, GL_COMPILE_STATUS, &is_compiled);

  if (is_compiled == GL_FALSE) {
    GLint max_length{0};
    glGetShaderiv(shader_handle_tmp, GL_INFO_LOG_LENGTH, &max_length);

    // The maxLength includes the NULL character
    GLsizei returnd_info_size{0};
    std::array<GLchar, SURGE_OPENGL_ERROR_BUFFER_SIZE> info_log{};

    glGetShaderInfoLog(shader_handle_tmp, SURGE_OPENGL_ERROR_BUFFER_SIZE, &returnd_info_size,
                       info_log.data());

    // We don't need the shader anymore.
    glDeleteShader(shader_handle_tmp);

#ifdef SURGE_SYSTEM_Windows
    log_error(L"Shader \"{}\" handle {} compilation failed:", p.c_str(), shader_handle_tmp);
    log_error("Shader compilation error: {}", info_log.data());
#else
    log_error("Shader \"{}\" handle {} compilation failed:\n  {}", p.c_str(), shader_handle_tmp,
              info_log.data());
#endif

    return {};
  } else {
#ifdef SURGE_SYSTEM_Windows
    log_info(L"Shader \"{}\" handle {} compilation succesfull", p.c_str(), shader_handle_tmp);
#else
    log_info("Shader \"{}\" handle {} compilation succesfull", p.c_str(), shader_handle_tmp);
#endif
    return shader_handle_tmp;
  }
}

auto surge::link_shader_program(GLuint vertex_shader_handle, GLuint fragment_shader_handle,
                                bool destroy_shaders) noexcept -> std::optional<GLuint> {

  log_info("Linking shader handles {} and {}.", vertex_shader_handle, fragment_shader_handle);

  // Get a program object.
  GLuint program_handle_tmp{glCreateProgram()};

  // Attach our shaders to our program
  glAttachShader(program_handle_tmp, vertex_shader_handle);
  glAttachShader(program_handle_tmp, fragment_shader_handle);

  // Link our program
  glLinkProgram(program_handle_tmp);

  GLint is_linked{0};
  glGetProgramiv(program_handle_tmp, GL_LINK_STATUS, &is_linked);

  if (is_linked == GL_FALSE) {
    GLint max_length = 0;
    glGetProgramiv(program_handle_tmp, GL_INFO_LOG_LENGTH, &max_length);

    // The max_length includes the NULL character
    GLsizei returnd_info_size{0};
    std::array<GLchar, SURGE_OPENGL_ERROR_BUFFER_SIZE> info_log{};

    glGetProgramInfoLog(program_handle_tmp, SURGE_OPENGL_ERROR_BUFFER_SIZE, &returnd_info_size,
                        info_log.data());

    // Cleanup
    glDetachShader(program_handle_tmp, vertex_shader_handle);
    glDetachShader(program_handle_tmp, fragment_shader_handle);
    glDeleteProgram(program_handle_tmp);

    log_info("Failed to link shader handles {} and {} to create program:\n  {}",
             vertex_shader_handle, fragment_shader_handle, info_log.data());
    return {};

  } else {

    glDetachShader(program_handle_tmp, vertex_shader_handle);
    glDetachShader(program_handle_tmp, fragment_shader_handle);

    if (destroy_shaders) {
      log_info("Destroying shaders handles {} and {}", vertex_shader_handle,
               fragment_shader_handle);
      glDeleteShader(vertex_shader_handle);
      glDeleteShader(fragment_shader_handle);
    }

    log_info("Shader handles {} and {} linked succesfully. Program handle: {}",
             vertex_shader_handle, fragment_shader_handle, program_handle_tmp);

    return program_handle_tmp;
  }
}

auto surge::create_program(const std::filesystem::path &vertex_shader_path,
                           const std::filesystem::path &fragment_shader_path) noexcept
    -> std::optional<GLuint> {

  const auto vertex_shader_handle{load_and_compile(vertex_shader_path, GL_VERTEX_SHADER)};
  const auto fragment_shader_handle{load_and_compile(fragment_shader_path, GL_FRAGMENT_SHADER)};

  if (vertex_shader_handle && fragment_shader_handle) {
    return link_shader_program(*vertex_shader_handle, *fragment_shader_handle);
  } else {
    return {};
  }
}

auto surge::create_program(const char *vss, const char *fss, const char *vsn,
                           const char *fsn) noexcept -> std::optional<GLuint> {

  const auto vertex_shader_handle{load_and_compile(vss, vsn, GL_VERTEX_SHADER)};
  const auto fragment_shader_handle{load_and_compile(fss, fsn, GL_FRAGMENT_SHADER)};

  if (vertex_shader_handle && fragment_shader_handle) {
    return link_shader_program(*vertex_shader_handle, *fragment_shader_handle);
  } else {
    return {};
  }
}