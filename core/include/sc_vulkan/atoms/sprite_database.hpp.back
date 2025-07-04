#ifndef SURGE_CORE_ATOM_SPRITE_DATABASE_HPP
#define SURGE_CORE_ATOM_SPRITE_DATABASE_HPP

#include "sc_error_types.hpp"
#include "sc_vulkan/sc_vulkan_descriptor.hpp"
#include "sc_vulkan/sc_vulkan_types.hpp"

#include <glm/glm.hpp>
#include <span>

namespace surge::renderer::vk::atom::sprite_database {

// Sprite database data
struct SpriteDatabaseT;

/**
 * Handle to a sprite database
 * A sprite database is a structure that contains and manages sprites.
 * Sprites are textured quads that have a size and a position. Sprites in the database can also be
 * animated, i.e., have textures that change with time.
 */
using SpriteDatabase = SpriteDatabaseT *;

/**
 * Controls the blending mode of the sprite database
 */
enum class SpriteDatabaseBlendingMode { none, additive, alpha };

/**
 * Controls the creation of a sprite database
 */
struct SpriteDatabaseCreateInfo {
  Context ctx{nullptr};                                                       // Vulkan context
  SpriteDatabaseBlendingMode blending_mode{SpriteDatabaseBlendingMode::none}; // Blending mode
  usize max_sprites{0}; // Max. number of sprites that can be contained in the database
};

/**
 * Specifies the sprite data to be added to the database
 */
struct SpriteDatabasePushInfo {
  glm::vec2 position{0.0f};         // Sprite position
  glm::vec2 scale{0.0f};            // Sprite size
  float z{0.0f};                    // Sprite z position
  glm::vec4 color_multiplier{1.0f}; // Sprite color modifier.
};

/**
 * Specifies the sprite data to add it to the database
 */
struct SpriteDatabaseAddInfo {
  Context ctx{nullptr};              // Vulkan context
  const char *texture_path{nullptr}; // Path to sprite texture
  glm::vec2 position{0.0f};          // Initial sprite position.
  glm::vec2 scale{0.0f};             // Initial sprite size.
  float z{0.0f};                     // Initial sprite z position.
  glm::vec4 color_multiplier{1.0f};  // Initial sprite color modifier.
};

/**
 * Creates a new sprite database
 * @param ci The sprite creation info. structure.
 * @param desc_alloc A descriptor allocator that can support at least one combined image sampler
 * @return A sprite database or an error code.
 */
auto create(const SpriteDatabaseCreateInfo &ci, GrowableDescriptorAllocator &desc_alloc)
    -> Result<SpriteDatabase>;

/**
 * Destroy a sprite database
 * @param database Sprite database to destroy
 * @param ctx Vulkan context
 */
void destroy(SpriteDatabase database, Context ctx);

/**
 * Add a new sprite to the database
 * @param database Sprite database to add sprite to.
 * @param add_info Data related to the new sprite to be added.
 * @return Nothing in case of success or error code in case of failure.
 */
auto add(SpriteDatabase database, SpriteDatabaseAddInfo add_info) -> Result<void>;

void push(SpriteDatabase database, const SpriteDatabasePushInfo &push_info);
auto sync(SpriteDatabase database, Context ctx) -> Result<void>;
void draw(SpriteDatabase database, Context ctx, const glm::mat4 &projection_matrix,
          const glm::mat4 &view_matrix);

auto make_ortho_projection(const glm::vec2 &dims) -> glm::mat4;
auto make_view(const glm::vec2 &eye) -> glm::mat4;
auto make_model_matrix(const glm::vec2 &position, const glm::vec2 &scale, float z = 1.0f)
    -> glm::mat4;

} // namespace surge::renderer::vk::atom::sprite_database

#endif // SURGE_CORE_ATOM_SPRITE_DATABASE_HPP