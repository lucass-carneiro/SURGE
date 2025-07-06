#ifndef SURGE_CORE_ATOM_SPRITE_DATABASE_HPP
#define SURGE_CORE_ATOM_SPRITE_DATABASE_HPP

#include "sc_error_types.hpp"
#include "sc_vulkan/sc_vulkan_descriptor.hpp"
#include "sc_vulkan/sc_vulkan_types.hpp"

#include <glm/glm.hpp>
#include <span>

namespace surge::renderer::vk::atom::sprite_database {

// Sprite database data
struct SpriteDatabaseImpl;

/**
 * Handle to a sprite database
 * A sprite database is a structure that contains and manages sprites.
 * Sprites are textured quads that have a size and a position. Sprites in the database can also be
 * animated, i.e., have textures that change with time.
 */
using SpriteDatabase = SpriteDatabaseImpl *;

/**
 * Controls the blending mode of the sprite database
 */
enum class BlendingMode { none, additive, alpha };

/**
 * Controls the creation of a sprite database
 */
struct CreateInfo {
  BlendingMode blending_mode{BlendingMode::none}; // Blending mode
  usize max_sprites{0};                           // Max. number of sprites in the database
};

struct UpdateInfo {
  glm::vec2 position{0.0f};
  glm::vec2 scale{0.0f};
  float z{0.0}; // (1.0 = near, 0.0 = far)
  usize texture_id{0};
  glm::vec4 color_multiplier{1.0f};
};

/**
 * Creates an orthographic projection for 2D rendering. It places the origin on upper left corner
 * of the game window.
 * @param dims Screen dimensions.
 * @return The 2D projection matrix.
 */
auto make_ortho_projection(const glm::vec2 &dims) -> glm::mat4;

/**
 * Creates a view matrix for 2D rendering
 * @param eye Position of the camera in 2D coordinates.
 * @return The 2D view matrix.
 */
auto make_view(const glm::vec2 &eye) -> glm::mat4;

/**
 *
 * @param position Sprite position.
 * @param scale Sprite scale
 * @param z Sprite depth (1.0 = near, 0.0 = far)
 * @return The model matrix for the sprite
 */
auto make_model_matrix(const glm::vec2 &position, const glm::vec2 &scale, float z = 1.0f)
    -> glm::mat4;

/**
 * Creates a new sprite database
 * @param ci The sprite creation info. structure.
 * @param desc_alloc A descriptor allocator that can support at least one combined image sampler
 * @return A sprite database or an error code.
 */
auto create(Context ctx, const CreateInfo &ci, GrowableDescriptorAllocator &desc_alloc)
    -> Result<SpriteDatabase>;

/**
 * Destroy a sprite database
 * @param database Sprite database to destroy
 * @param ctx Vulkan context
 */
void destroy(Context ctx, SpriteDatabase database);

/**
 * Reads images from disk and uploads them to the GPU.
 * This function blocks until the transfer is complete.
 * @param ctx Vulkan context
 * @param database Sprite database
 * @param paths Paths to images to upload
 * @return Nothing if success, error otherwise.
 */
auto upload_images(Context ctx, SpriteDatabase database, std::span<const char *> paths)
    -> Result<void>;

/**
 * Updates draw data for a single sprite
 * @param ctx Vulkan context
 * @param database Sprite database
 * @param update_info UpdateInfo structure with update data
 */
void update_draw_data(SpriteDatabase database, const UpdateInfo &update_info);

/**
 * Updates draw data for multiple sprites.
 * @param ctx Vulkan context
 * @param database Sprite database
 * @param update_infos UpdateInfo structures with update data
 */
void update_draw_data(SpriteDatabase database, std::span<const UpdateInfo> update_infos);

/**
 * Signals the end of sprite data updates and uploads changes to GPU
 * @param ctx Vulkan Context
 * @param database Sprite database
 * @return Nothing if success, error otherwise.
 */
auto synchronize_draw_data(Context ctx, SpriteDatabase database) -> Result<void>;

/**
 * Draws the sprite database
 * TODO: The projection matrix should not be specified every draw call
 * @param ctx Vulkan context
 * @param database Sprite database
 * @param projection_matrix The orthographic projection matrix
 * @param view_matrix The camera view matrix
 */
void draw(Context ctx, SpriteDatabase database, const glm::mat4 &projection_matrix,
          const glm::mat4 &view_matrix);

} // namespace surge::renderer::vk::atom::sprite_database

#endif // SURGE_CORE_ATOM_SPRITE_DATABASE_HPP