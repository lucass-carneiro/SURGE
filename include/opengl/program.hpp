#ifndef SURGE_OPENGL_PROGRAM_HPP
#define SURGE_OPENGL_PROGRAM_HPP

#include "headers.hpp"

#include <filesystem>
#include <optional>

namespace surge {

[[nodiscard]] auto load_and_compile(const char *shader_source, const char *shader_name,
                                    GLenum shader_type) noexcept -> std::optional<GLuint>;

[[nodiscard]] auto load_and_compile(const std::filesystem::path &p, GLenum shader_type) noexcept
    -> std::optional<GLuint>;

[[nodiscard]] auto link_shader_program(GLuint vertex_shader_handle, GLuint fragment_shader_handle,
                                       bool destroy_shaders = true) noexcept
    -> std::optional<GLuint>;

[[nodiscard]] auto create_program(const std::filesystem::path &vertex_shader_path,
                                  const std::filesystem::path &fragment_shader_path) noexcept
    -> std::optional<GLuint>;

[[nodiscard]] auto create_program(const char *vss, const char *fss, const char *vsn,
                                  const char *fsn) noexcept -> std::optional<GLuint>;

} // namespace surge

#endif // SURGE_OPENGL_PROGRAM_HPP