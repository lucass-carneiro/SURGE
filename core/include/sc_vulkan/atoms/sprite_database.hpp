#ifndef SURGE_CORE_ATOM_SPRITE_DATABASE_HPP
#define SURGE_CORE_ATOM_SPRITE_DATABASE_HPP

#include "sc_error_types.hpp"
#include "sc_vulkan/sc_vulkan_types.hpp"

#include <glm/glm.hpp>

namespace surge::renderer::vk::atom::sprite_database {

struct SpriteDatabaseT;
using SpriteDatabase = SpriteDatabaseT *;

struct WorldMatrices {
  glm::mat4 projection{1.0};
  glm::mat4 view{1.0};
  glm::mat4 model{1.0};
};

auto create(Context ctx) -> Result<SpriteDatabase>;
void destroy(SpriteDatabase database, Context ctx);

void draw(SpriteDatabase database, Context ctx, const WorldMatrices &world_matrices);

auto make_ortho_projection(const glm::vec2 &dims) -> glm::mat4;
auto make_view(const glm::vec2 &eye) -> glm::mat4;
auto make_model_matrix(const glm::vec2 &position, const glm::vec2 &scale, float z = 1.0f)
    -> glm::mat4;

} // namespace surge::renderer::vk::atom::sprite_database

#endif // SURGE_CORE_ATOM_SPRITE_DATABASE_HPP