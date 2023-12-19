#ifndef SURGE_ATOM_STATIC_IMAGE
#define SURGE_ATOM_STATIC_IMAGE

#include "renderer.hpp"

#include <glm/fwd.hpp>

/**
 * Drawable static image quad
 */
namespace surge::atom::static_image {

template <typename T> using vec = eastl::vector<T, allocators::eastl::gp_allocator>;
template <typename T, std::size_t N> using arr = std::array<T, N>;

enum class error : std::uint32_t { load_error = 1, stbi_error = 2, shader_creation = 3 };

struct one_buffer_data {
  glm::vec2 dimentions{0.0f};
  glm::vec2 ds{0.0f};

  GLuint texture_id;
  GLuint VAO;
};

template <std::size_t N> struct st_buffer_data {
  arr<glm::vec2, N> dimentions;
  arr<glm::vec2, N> ds;

  arr<GLuint, N> texture_ids;
  arr<GLuint, N> VAOs;
};

struct buffer_data {
  vec<glm::vec2> dimentions;
  vec<glm::vec2> ds;

  vec<GLuint> texture_ids;
  vec<GLuint> VAOs;
};

struct one_draw_data {
  glm::mat4 projection;
  glm::mat4 view;
  glm::vec3 pos;
  glm::vec3 scale;
  glm::vec2 region_origin;
  glm::vec2 region_dims;
  bool h_flip{false};
  bool v_flip{false};
};

template <std::size_t N> struct st_draw_data {
  arr<glm::mat4, N> projection;
  arr<glm::mat4, N> view;
  arr<glm::vec3, N> pos;
  arr<glm::vec3, N> scale;
  arr<glm::vec2, N> region_origin;
  arr<glm::vec2, N> region_dims;
  arr<bool, N> h_flip{false};
  arr<bool, N> v_flip{false};
};

struct draw_data {
  vec<glm::mat4> projection;
  vec<glm::mat4> view;
  vec<glm::vec3> pos;
  vec<glm::vec3> scale;
  vec<glm::vec2> region_origin;
  vec<glm::vec2> region_dims;
  vec<bool> h_flip{false};
  vec<bool> v_flip{false};
};

auto create(const char *p,
            renderer::texture_filtering filtering = renderer::texture_filtering::linear) noexcept
    -> tl::expected<one_buffer_data, error>;

void draw(GLuint shader_program, const one_buffer_data &ctx, const one_draw_data &dctx) noexcept;
void draw(GLuint shader_program, const one_buffer_data &&ctx, one_draw_data &&dctx) noexcept;

} // namespace surge::atom::static_image

#endif // SURGE_ATOM_STATIC_IMAGE