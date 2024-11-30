#ifndef SURGE_CORE_GL_ATOM_TEXT_DATABASE_HPP
#define SURGE_CORE_GL_ATOM_TEXT_DATABASE_HPP

#include "sc_error_types.hpp"
#include "sc_integer_types.hpp"

#include <span>
#include <tl/expected.hpp>

namespace surge::gl_atom::text_database {

namespace glyph_cache {

enum class languages : int { en_US, pt_BR };

using font_size_t = signed long;
using font_res_t = unsigned int;

struct create_info {
  const std::span<const char *> fonts{};
  const std::span<font_size_t> sizes_in_pts{};
  const std::span<font_res_t> resolution_dpis{};
  const std::span<languages> langs{};
  bool make_all_resident{false};
  unsigned long replacement_char_code{0x0000FFFD};
};

struct cache_t;
using cache = cache_t *;

auto create(create_info ci) -> tl::expected<cache, error>;
void destroy(cache gc);

void reside_all(cache gc);
void unreside_all(cache gc);

} // namespace glyph_cache

} // namespace surge::gl_atom::text_database

#endif // SURGE_CORE_GL_ATOM_TEXT_DATABASE_HPP