#ifndef SURGE_SHADER_HPP
#define SURGE_SHADER_HPP

#include "window.hpp"

namespace surge {

class shader {
public:
  shader(const char *n, GLenum t, const char *src)
      : name{n}, source{src}, type{t} {}

  [[nodiscard]] inline auto is_valid() const noexcept -> bool {
    return handle.has_value();
  }

  [[nodiscard]] inline auto get_name() const noexcept -> const char * {
    return name;
  }

  [[nodiscard]] inline auto get_source() const noexcept -> const char * {
    return source;
  }

  [[nodiscard]] inline auto get_type() const noexcept -> GLenum { return type; }

  [[nodiscard]] inline auto get_handle() const noexcept
      -> std::optional<GLuint> {
    return handle;
  }

  void compile() noexcept;

private:
  const char *name;
  const char *source;
  const GLenum type;
  std::optional<GLuint> handle{};
};

auto link_shaders(shader &vertex_shader, shader &fragment_shader) noexcept
    -> std::optional<GLuint>;

} // namespace surge

#endif // SURGE_SHADER_HPP