#ifndef DTU_TYPE_ALIASES_HPP
#define DTU_TYPE_ALIASES_HPP

#include "player/container_types.hpp"
#include "player/sprite.hpp"
#include "player/text.hpp"
#include "player/texture.hpp"

namespace DTU {

using cmd_code_t = surge::u32;
using cmdq_t = surge::deque<cmd_code_t>;

using tdb_t = surge::atom::texture::database;
using sdb_t = surge::atom::sprite::database;

struct txd_t {
  surge::atom::text::text_engine ten{};
  surge::atom::text::glyph_cache gc0{};
  surge::atom::text::glyph_cache gc1{};
  surge::atom::text::text_buffer txb{};
  glm::vec4 draw_color{1.0f};
};

} // namespace DTU

#endif // DTU_TYPE_ALIASES_HPP