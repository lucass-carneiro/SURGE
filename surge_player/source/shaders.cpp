#include "files.hpp"
#include "logging.hpp"
#include "renderer.hpp"

static auto load_and_compile_shader(const char *p, GLenum shader_type) noexcept
    -> std::optional<GLuint> {
  using namespace surge;
  log_info("Loading shader file %s", p);

  files::load_file_return_t file{};

  if (shader_type == GL_VERTEX_SHADER || shader_type == GL_FRAGMENT_SHADER) {
    file = files::load_file(p, true);
  } else {
    log_error("Unrecognized shader type %u", shader_type);
    return {};
  }

  if (!file) {
    log_error("Unable to load shader file %s", p);
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
  files::free_file(*file);

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

    log_error("Shader %s handle %u compilation failed:\n  %s", p, shader_handle_tmp,
              info_log.data());

    return {};
  } else {
    log_info("Shader %s handle %u compilation succesfull", p, shader_handle_tmp);
    return shader_handle_tmp;
  }
}

static auto link_shader_program(GLuint vertex_shader_handle, GLuint fragment_shader_handle,
                                bool destroy_shaders = true) noexcept -> std::optional<GLuint> {

  log_info("Linking shader handles %u and %u.", vertex_shader_handle, fragment_shader_handle);

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

    log_info("Failed to link shader handles %u and %u to create program:\n  %s",
             vertex_shader_handle, fragment_shader_handle, info_log.data());
    return {};

  } else {

    glDetachShader(program_handle_tmp, vertex_shader_handle);
    glDetachShader(program_handle_tmp, fragment_shader_handle);

    if (destroy_shaders) {
      log_info("Destroying shaders handles %u and %u", vertex_shader_handle,
               fragment_shader_handle);
      glDeleteShader(vertex_shader_handle);
      glDeleteShader(fragment_shader_handle);
    }

    log_info("Shader handles %u and %u linked succesfully. Program handle: %u",
             vertex_shader_handle, fragment_shader_handle, program_handle_tmp);

    return program_handle_tmp;
  }
}

auto surge::renderer::create_shader_program(const char *vertex_shader_path,
                                            const char *fragment_shader_path) noexcept
    -> std::optional<GLuint> {

  const auto vertex_shader_handle{load_and_compile_shader(vertex_shader_path, GL_VERTEX_SHADER)};
  const auto fragment_shader_handle{
      load_and_compile_shader(fragment_shader_path, GL_FRAGMENT_SHADER)};

  if (vertex_shader_handle && fragment_shader_handle) {
    return link_shader_program(*vertex_shader_handle, *fragment_shader_handle);
  } else {
    return {};
  }
}