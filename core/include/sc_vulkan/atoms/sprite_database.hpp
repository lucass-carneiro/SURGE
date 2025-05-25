#ifndef SURGE_CORE_ATOM_SPRITE_DATABASE_HPP
#define SURGE_CORE_ATOM_SPRITE_DATABASE_HPP

#include "sc_error_types.hpp"
#include "sc_vulkan/sc_vulkan_types.hpp"

#include <glm/glm.hpp>

namespace surge::renderer::vk::atom::sprite_database {

struct SpriteDatabaseT;
using SpriteDatabase = SpriteDatabaseT *;

auto create(Context ctx, usize max_sprites) -> Result<SpriteDatabase>;
void destroy(SpriteDatabase database, Context ctx);

void push(SpriteDatabase database, const glm::vec2 &position, const glm::vec2 &scale,
          float z = 1.0f);
auto sync(SpriteDatabase database, Context ctx) -> Result<void>;
void draw(SpriteDatabase database, Context ctx, const glm::mat4 &projection_matrix,
          const glm::mat4 &view_matrix);

auto make_ortho_projection(const glm::vec2 &dims) -> glm::mat4;
auto make_view(const glm::vec2 &eye) -> glm::mat4;
auto make_model_matrix(const glm::vec2 &position, const glm::vec2 &scale, float z = 1.0f)
    -> glm::mat4;

} // namespace surge::renderer::vk::atom::sprite_database

#endif // SURGE_CORE_ATOM_SPRITE_DATABASE_HPP