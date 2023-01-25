#ifndef SURGE_ACTOR_HPP
#define SURGE_ACTOR_HPP

#include "sprite.hpp"

namespace surge {

class actor {
public:
  template <surge_allocator alloc_t>
  actor(alloc_t *allocator, const std::filesystem::path &sprite_sheet_path,
        const char *sprite_sheet_ext)
      : actor_sprite{allocator, sprite_sheet_path, sprite_sheet_ext} {}

private:
  sprite actor_sprite;
};

} // namespace surge

#endif