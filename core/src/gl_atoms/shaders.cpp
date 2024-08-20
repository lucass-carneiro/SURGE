#include "gl_atoms/shaders.hpp"

#include "files.hpp"
#include "logging.hpp"

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>

#endif

#include <array>

static auto load_and_compile_shader(const char *p, GLenum shader_type) noexcept
    -> tl::expected<GLuint, surge::error> {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::shader::load_and_compile_shader");
  TracyGpuZone("GPU load_and_compile_shader");
#endif

  using namespace surge;
  log_info("Loading shader file {}", p);

  files::file file{};

  if (shader_type == GL_VERTEX_SHADER || shader_type == GL_FRAGMENT_SHADER
      || shader_type == GL_COMPUTE_SHADER) {
    file = files::load_file(p, true);
  } else {
    log_error("Unrecognized shader type {}", shader_type);
    return tl::unexpected(surge::error::unrecognized_shader);
  }

  if (!file) {
    log_error("Unable to load shader file {}", p);
    return tl::unexpected(surge::error::shader_load_error);
  }

  // Get the source code in GL format
  auto file_source{static_cast<const GLchar *>(static_cast<void *>(file.value().data()))};

  // Create an empty shader handle
  const GLuint shader_handle_tmp = glCreateShader(shader_type);

  // Send the shader source code to GL
  glShaderSource(shader_handle_tmp, 1, &file_source, nullptr);

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

    log_error("Shader {} handle {} compilation failed:\n  {}", p, shader_handle_tmp,
              info_log.data());

    return tl::unexpected(surge::error::shader_load_error);
  } else {
    log_info("Shader {} handle {} compilation succesfull", p, shader_handle_tmp);
    return shader_handle_tmp;
  }
}

static auto link_shader_program(GLuint vertex_shader_handle, GLuint fragment_shader_handle,
                                bool destroy_shaders = true) noexcept
    -> tl::expected<GLuint, surge::error> {

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::shader::link_shader_program");
  TracyGpuZone("GPU surge::gl_atom::shader::link_shader_program");
#endif

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
    return tl::unexpected(surge::error::shader_link_error);

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

static auto link_single_shader(GLuint shader_handle, bool destroy_shaders = true) noexcept
    -> tl::expected<GLuint, surge::error> {

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::shader::link_shader_program");
  TracyGpuZone("GPU surge::gl_atom::shader::link_shader_program");
#endif

  log_info("Linking shader handle {}", shader_handle);

  // Get a program object.
  GLuint program_handle_tmp{glCreateProgram()};

  // Attach our shaders to our program
  glAttachShader(program_handle_tmp, shader_handle);

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
    glDetachShader(program_handle_tmp, shader_handle);
    glDeleteProgram(program_handle_tmp);

    log_info("Failed to link shader handles {} to create program:\n  {}", shader_handle,
             info_log.data());
    return tl::unexpected(surge::error::shader_link_error);

  } else {
    glDetachShader(program_handle_tmp, shader_handle);

    if (destroy_shaders) {
      log_info("Destroying shaders handles {}", shader_handle);
      glDeleteShader(shader_handle);
    }

    log_info("Shader handle {} linked succesfully. Program handle: {}", shader_handle,
             program_handle_tmp);

    return program_handle_tmp;
  }
}

auto surge::gl_atom::shader::create_shader_program(const char *vertex_shader_path,
                                                   const char *fragment_shader_path) noexcept
    -> tl::expected<GLuint, error> {

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::shader::create_shader_program");
#endif

  const auto vertex_shader_handle{load_and_compile_shader(vertex_shader_path, GL_VERTEX_SHADER)};
  const auto fragment_shader_handle{
      load_and_compile_shader(fragment_shader_path, GL_FRAGMENT_SHADER)};

  if (!vertex_shader_handle) {
    return tl::unexpected(vertex_shader_handle.error());
  }

  if (!fragment_shader_handle) {
    return tl::unexpected(fragment_shader_handle.error());
  }

  return link_shader_program(*vertex_shader_handle, *fragment_shader_handle);
}

auto surge::gl_atom::shader::create_compute_shader(const char *compute_shader_path) noexcept
    -> tl::expected<GLuint, error> {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::shader::create_compute_shader");
#endif

  const auto compute_shader_handle{load_and_compile_shader(compute_shader_path, GL_COMPUTE_SHADER)};

  if (!compute_shader_handle) {
    return tl::unexpected{compute_shader_handle.error()};
  }

  return link_single_shader(*compute_shader_handle);
}

void surge::gl_atom::shader::destroy_shader_program(GLuint program) noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::shader::destroy_shader_program");
  TracyGpuZone("GPU surge::gl_atom::shader::destroy_shader_program");
#endif
  log_info("Destroying shader program handle {}", program);
  glDeleteProgram(program);
}

void surge::gl_atom::shader::dispatch_compute(GLuint program, GLuint x, GLuint y,
                                              GLuint z) noexcept {
  glUseProgram(program);
  glDispatchCompute(x, y, z);
}

void surge::gl_atom::shader::dispatch_compute(GLuint program, GLuint x, GLuint y, GLuint z,
                                              GLbitfield barriers) noexcept {
  dispatch_compute(program, x, y, z);
  glMemoryBarrier(barriers);
}