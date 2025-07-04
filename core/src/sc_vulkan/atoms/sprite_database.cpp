#include "sc_vulkan/atoms/sprite_database.hpp"

#include "sc_logging.hpp"
#include "sc_vulkan/sc_vulkan_images.hpp"
#include "sc_vulkan/sc_vulkan_malloc.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

auto surge::renderer::vk::atom::sprite_database::make_ortho_projection(const glm::vec2 &dims)
    -> glm::mat4 {
  return glm::ortho(0.0f, dims[0], 0.0f, dims[1], 1.0f, 0.0f);
}

auto surge::renderer::vk::atom::sprite_database::make_view(const glm::vec2 &eye) -> glm::mat4 {
  return glm::lookAt(glm::vec3{eye, 1.0f}, glm::vec3{eye, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f});
}

auto surge::renderer::vk::atom::sprite_database::make_model_matrix(const glm::vec2 &position,
                                                                   const glm::vec2 &scale, float z)
    -> glm::mat4 {
  const auto mv{glm::vec3{position, z}};
  const auto sc{glm::vec3{scale, 1.0f}};
  return glm::scale(glm::translate(glm::mat4{1.0f}, mv), sc);
}

struct surge::renderer::vk::atom::sprite_database::SpriteDatabaseImpl {
  usize max_sprites{0};      // How many sprites we can add.
  usize curr_num_sprites{0}; // How many sprites we added pushed so far.

  containers::mimalloc::Vector<Buffer> img_src_buffers{};        // CPU image buffer
  containers::mimalloc::Vector<AllocatedImage> img_dst_images{}; // GPU image data
};

auto surge::renderer::vk::atom::sprite_database::create(Context ctx, const CreateInfo &ci,
                                                        GrowableDescriptorAllocator &desc_alloc)
    -> Result<SpriteDatabase> {
  log_info("Creating sprite database");

  // Allocate object handle
  auto database{
      static_cast<SpriteDatabase>(allocators::mimalloc::malloc(sizeof(SpriteDatabaseImpl)))};

  if (database == nullptr) {
    log_error("Unable to allocate sprite database");
    return Err{vk_atom_sprite_database_init};
  }

  new (database) SpriteDatabaseImpl();

  // Database handle as void*, used for printing
  const auto dbh{static_cast<void *>(database)};

  // Basic initialization
  database->max_sprites = ci.max_sprites;

  // Vector reserving
  database->img_src_buffers.reserve(database->max_sprites);
  database->img_dst_images.reserve(database->max_sprites);

  // Creation done
  log_info("Created sprite database, handle {}", dbh);
  return database;
}

void surge::renderer::vk::atom::sprite_database::destroy(Context ctx, SpriteDatabase database) {
  log_info("Destroying sprite database, handle {}", static_cast<void *>(database));

  // Destroy Vulkan objects
  vkDeviceWaitIdle(ctx->device);

  // Destroy CPU images
  for (const auto &buffer : database->img_src_buffers) {
    destroy_buffer(ctx, buffer);
  }
  database->img_src_buffers.~vector();

  // Destroy GPU images
  for (const auto &img : database->img_dst_images) {
    destroy_image(ctx, img);
  }
  database->img_dst_images.~vector();

  // Destroy handle
  allocators::mimalloc::free(database);
}

auto surge::renderer::vk::atom::sprite_database::upload_images(Context ctx, SpriteDatabase database,
                                                               std::span<const char *> paths)
    -> Result<void> {
  const auto dbh{static_cast<void *>(database)};

  // Will the final size be bounded by database->max_sprites?
  const bool cond_a{(database->img_dst_images.size() + paths.size()) > database->max_sprites};

  // TODO: Adding conditions: We can add if there is space in the src buffer and the final size
  // would not biolate the max size requirement
  if ((database->img_dst_images.size() + paths.size()) > database->max_sprites) {
    log_error("Unable to upload more images to sprite database {}. Database full", dbh);
    return Err{vk_atom_sprite_database_full};
  }

  // Create buffers and store handles in vectors
  for (const auto &path : paths) {
    // TODO: Read data from actual image
    const auto black{glm::packUnorm4x8(glm::vec4(0, 0, 0, 1))};
    const auto white{glm::packUnorm4x8(glm::vec4(1, 1, 1, 1))};

    std::array<u32, 16 * 16> img_data{};
    for (usize x = 0; x < 16; x++) {
      for (usize y = 0; y < 16; y++) {
        img_data[y * 16 + x] = ((x % 2) ^ (y % 2)) ? white : black;
      }
    }

    VkExtent3D img_extent{16, 16, 1};

    // Create source buffer
    const auto total_image_size{img_extent.depth * img_extent.width * img_extent.height * 4};
    auto img_src_buffer{create_buffer(ctx, total_image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      VMA_MEMORY_USAGE_CPU_TO_GPU)};

    if (!img_src_buffer) {
      log_error("Sprite database {} unable to create source buffer for texture {}", dbh, path);
      return Err{img_src_buffer.error()};
    }

    // Transfer image data to source buffer
    memcpy(img_src_buffer->info.pMappedData, img_data.data(), total_image_size);

    // Save the buffer to source vector
    database->img_src_buffers.push_back(*img_src_buffer);

    // Create destination buffer
    auto img_dest_image{create_image(ctx, img_extent, VK_FORMAT_R8G8B8A8_UNORM,
                                     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                                         | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                     false)};

    if (!img_dest_image) {
      log_error("Sprite database {} unable to create GPU memory for texture {}", dbh, path);
      return Err{img_dest_image.error()};
    }

    database->img_dst_images.push_back(*img_dest_image);
  }

  // Do copy. The rule is: if there are things in the src buffer, then transfer them to the target
  // buffer

  return {};
}