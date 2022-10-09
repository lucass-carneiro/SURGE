#ifndef SURGE_CREATE_PROGRAM_HPP
#define SURGE_CREATE_PROGRAM_HPP

#include "link_program.hpp"

namespace surge {

template <surge_allocator alloc_t>
[[nodiscard]] inline auto create_program(alloc_t *allocator,
                                         // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
                                         const std::filesystem::path &vertex_shader_path,
                                         const std::filesystem::path &fragment_shader_path) noexcept
    -> std::optional<GLuint> {

  const auto vertex_shader_handle{
      load_and_compile<GL_VERTEX_SHADER>(allocator, vertex_shader_path)};
  const auto fragment_shader_handle{
      load_and_compile<GL_FRAGMENT_SHADER>(allocator, fragment_shader_path)};

  if (vertex_shader_handle && fragment_shader_handle) {
    return link_shader_program(*vertex_shader_handle, *fragment_shader_handle);
  } else {
    return {};
  }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
[[nodiscard]] inline auto create_program(const char *vss, const char *fss, const char *vsn,
                                         const char *fsn) noexcept -> std::optional<GLuint> {

  const auto vertex_shader_handle{load_and_compile<GL_VERTEX_SHADER>(vss, vsn)};
  const auto fragment_shader_handle{load_and_compile<GL_FRAGMENT_SHADER>(fss, fsn)};

  if (vertex_shader_handle && fragment_shader_handle) {
    return link_shader_program(*vertex_shader_handle, *fragment_shader_handle);
  } else {
    return {};
  }
}

} // namespace surge

#endif // SURGE_CREATE_PROGRAM_HPP