#ifndef DTU_TYPE_ALIASES_HPP
#define DTU_TYPE_ALIASES_HPP

#include "player/container_types.hpp"
#include "player/sprite.hpp"
#include "player/texture.hpp"

namespace DTU {

using cmd_code_t = surge::u32;
using cmdq_t = surge::deque<cmd_code_t>;

using tdb_t = surge::atom::texture::database;
using sdb_t = surge::atom::sprite::database;

} // namespace DTU

#endif // DTU_TYPE_ALIASES_HPP