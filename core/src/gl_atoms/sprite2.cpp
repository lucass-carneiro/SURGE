#include "gl_atoms/sprite2.hpp"

#include "allocators.hpp"
#include "logging.hpp"
struct surge::gl_atom::sprite2::database_t {
  usize max_sprites{0};
  usize buffer_redundancy{3};

  GLsync *fences{nullptr};

  GLuint buffer_id{0};
  sprite_info *buffer_data{nullptr};
};

auto surge::gl_atom::sprite2::database_create(database_create_info ci) noexcept
    -> tl::expected<database, error> {

  // Alloc instance
  auto sdb{static_cast<database>(allocators::mimalloc::malloc(sizeof(database_t)))};

  if (sdb == nullptr) {
    log_error("Unable to allocate sprite database instance");
    return tl::unexpected{sdb_instance_alloc};
  }

  new (sdb)(database_t)();

  // Read create info
  sdb->max_sprites = ci.max_sprites;
  sdb->buffer_redundancy = ci.buffer_redundancy;

  // Alloc fences array
  sdb->fences
      = static_cast<GLsync *>(allocators::mimalloc::calloc(sdb->buffer_redundancy, sizeof(GLsync)));

  if (sdb->fences == nullptr) {

    log_error("Unable to allocat sprite databes fence array");
    return tl::unexpected{sdb_fenc_alloc};
  }

  for (auto i = 0; i < sdb->buffer_redundancy; i++) {
    sdb->fences[i] = nullptr;
  }

  // Alloc GPU buffer
  const auto total_buffer_size{
      static_cast<GLsizeiptr>(sizeof(sprite_info) * sdb->max_sprites * sdb->buffer_redundancy)};

  constexpr GLbitfield map_flags{GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT};
  constexpr GLbitfield create_flags{map_flags | GL_DYNAMIC_STORAGE_BIT};

  glCreateBuffers(1, &(sdb->buffer_id));
  glNamedBufferStorage(sdb->buffer_id, total_buffer_size, nullptr, create_flags);
  sdb->buffer_data = static_cast<sprite_info *>(
      glMapNamedBufferRange(sdb->buffer_id, 0, total_buffer_size, map_flags));

  // Done
  log_info("Created new sprite database, handle {} using {} B of video memory",
           static_cast<void *>(sdb), total_buffer_size);

  return sdb;
}

void surge::gl_atom::sprite2::database_destroy(database sdb) noexcept {
  log_info("Destroying sprite database, handle {}", static_cast<void *>(sdb));
  // Free GPU buffer
  glUnmapNamedBuffer(sdb->buffer_id);
  glDeleteBuffers(1, &(sdb->buffer_id));

  // Fre fence array
  allocators::mimalloc::free(static_cast<void *>(sdb->fences));

  // Free instance
  allocators::mimalloc::free(static_cast<void *>(sdb));
}