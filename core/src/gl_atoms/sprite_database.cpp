#include "gl_atoms/sprite_database.hpp"

#include "allocators.hpp"
#include "gl_atoms/shaders.hpp"
#include "logging.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <gsl/gsl-lite.hpp>

/* This struct needs to be aligned such that
 * it is a multiple of `alignment`, where
 *
 * GLint alignment{0};
 * glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &alignment);
 */
struct sprite_info {
  GLuint64 texture_handle{0};
  float alpha{0.0f};
  float model[16]{
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
  };
  float view[4]{1.0f, 1.0f, 0.0f, 0.0f};
};

struct surge::gl_atom::sprite_database::database_t {
  usize max_sprites{0};
  usize buffer_redundancy{3};

  GLsync *fences{nullptr};

  GLuint buffer_id{0};
  sprite_info *buffer_data{nullptr};

  usize write_idx{0};
  usize write_buffer{0};
  GLuint sprite_shader{0};
  GLuint deep_sprite_shader{0};

  GLuint64 depth_texture_handle{0};
  GLuint64 depth_map_handle{0};
  glm::mat4 depth_model{1.0f};

  GLuint VBO{0};
  GLuint EBO{0};
  GLuint VAO{0};
};

void wait_buffer(surge::gl_atom::sprite_database::database sdb, surge::usize index) {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::sprite::wait_buffer");
  TracyGpuZone("GPU surge::gl_atom::sprite::wait_buffer");
#endif

  auto &fence{sdb->fences[index]};

  if (fence != nullptr) {
    while (true) {
      const auto wait_res{glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 1)};
      if (wait_res == GL_ALREADY_SIGNALED || wait_res == GL_CONDITION_SATISFIED) {
        glDeleteSync(fence);
        fence = nullptr;
        return;
      }
    }
  }
}

void lock_and_advance_buffer(surge::gl_atom::sprite_database::database sdb, surge::usize index) {
  auto &fence{sdb->fences[index]};

  if (fence == nullptr) {
    fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    sdb->write_buffer++;
    sdb->write_buffer %= sdb->buffer_redundancy;
    sdb->write_idx = 0;
  }
}

void surge::gl_atom::sprite_database::wait_idle(database sdb) noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::sprite::wait_idle");
  TracyGpuZone("GPU surge::gl_atom::sprite::wait_idle");
#endif

  for (usize i = 0; i < sdb->buffer_redundancy; i++) {
    wait_buffer(sdb, i);
  }
}

auto surge::gl_atom::sprite_database::create(database_create_info ci) noexcept
    -> tl::expected<database, error> {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::sprite::create");
  TracyGpuZone("GPU surge::gl_atom::sprite::create");
#endif

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

  for (usize i = 0; i < sdb->buffer_redundancy; i++) {
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

  // Compile shaders
  const auto sprite_shader{shader::create_shader_program("shaders/gl/sprite_database.vert",
                                                         "shaders/gl/sprite_database.frag")};
  if (!sprite_shader) {
    log_error("Unable to create sprite shader");
    return tl::unexpected{sprite_shader.error()};
  }

  const auto deep_sprite_shader{
      shader::create_shader_program("shaders/gl/deep_sprite.vert", "shaders/gl/deep_sprite.frag")};
  if (!deep_sprite_shader) {
    log_error("Unable to create deep sprite shader");
    return tl::unexpected{deep_sprite_shader.error()};
  }

  sdb->sprite_shader = *sprite_shader;
  sdb->deep_sprite_shader = *deep_sprite_shader;

  // Vertex buffers
  glCreateVertexArrays(1, &(sdb->VAO));
  glCreateBuffers(1, &(sdb->VBO));
  glCreateBuffers(1, &(sdb->EBO));

  const std::array<float, 20> vertex_attributes{
      0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // bottom left
      1.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
      1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
      0.0f, 0.0f, 0.0f, 0.0f, 1.0f, // top left
  };

  const std::array<GLuint, 6> draw_indices{0, 1, 2, 2, 3, 0};

  glBindVertexArray(sdb->VAO);

  glBindBuffer(GL_ARRAY_BUFFER, sdb->VBO);
  glBufferData(GL_ARRAY_BUFFER, vertex_attributes.size() * sizeof(float), vertex_attributes.data(),
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sdb->EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, draw_indices.size() * sizeof(GLuint), draw_indices.data(),
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        reinterpret_cast<const void *>(3 * sizeof(float))); // NOLINT

  // Done
  log_info("Created new sprite database, handle {} using {} B of video memory",
           static_cast<void *>(sdb), total_buffer_size);

  return sdb;
}

void surge::gl_atom::sprite_database::destroy(database sdb) noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::sprite::destroy");
  TracyGpuZone("GPU surge::gl_atom::sprite::destroy");
#endif

  log_info("Destroying sprite database, handle {}", static_cast<void *>(sdb));

  wait_idle(sdb);

  // Delete vertex buffers
  glDeleteBuffers(1, &(sdb->EBO));
  glDeleteBuffers(1, &(sdb->VBO));
  glDeleteVertexArrays(1, &(sdb->VAO));

  // Delete shader program
  shader::destroy_shader_program(sdb->sprite_shader);

  // Free GPU buffer
  glUnmapNamedBuffer(sdb->buffer_id);
  glDeleteBuffers(1, &(sdb->buffer_id));

  // Fre fence array
  allocators::mimalloc::free(static_cast<void *>(sdb->fences));

  // Free instance
  allocators::mimalloc::free(static_cast<void *>(sdb));
}

void surge::gl_atom::sprite_database::begin_add(database sdb) noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::sprite::begin_add");
  TracyGpuZone("GPU surge::gl_atom::sprite::begin_add");
#endif

  wait_buffer(sdb, sdb->write_buffer);
}

void surge::gl_atom::sprite_database::add(database sdb, GLuint64 texture_handle,
                                          const glm::mat4 &model_matrix, float alpha) noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::sprite::add");
#endif

  using std::memcpy;

  if (sdb->write_idx < sdb->max_sprites) {
    sprite_info si{};
    si.texture_handle = texture_handle;
    si.alpha = alpha;
    memcpy(si.model, glm::value_ptr(model_matrix), 16 * sizeof(float));

    const auto idx{sdb->write_buffer * sdb->max_sprites + sdb->write_idx};
    sdb->buffer_data[idx] = si;
    sdb->write_idx++;

  } else {
    log_warn("Sprite database {} capacity exceeded. Ignoring push request",
             static_cast<void *>(sdb));
  }
}

void surge::gl_atom::sprite_database::add(database sdb, GLuint64 texture_handle, glm::vec2 &&pos,
                                          glm::vec2 &&scale, float z, float alpha) noexcept {
  const auto mv{glm::vec3{std::move(pos), z}};
  const auto sc{glm::vec3{std::move(scale), 1.0f}};
  const auto model{glm::scale(glm::translate(glm::mat4{1.0f}, mv), sc)};
  add(sdb, texture_handle, model, alpha);
}

void surge::gl_atom::sprite_database::add(database sdb, GLuint64 texture_handle,
                                          const glm::vec2 &pos, const glm::vec2 &scale, float z,
                                          float alpha) noexcept {
  const auto mv{glm::vec3{pos, z}};
  const auto sc{glm::vec3{scale, 1.0f}};
  const auto model{glm::scale(glm::translate(glm::mat4{1.0f}, mv), sc)};
  add(sdb, texture_handle, model, alpha);
}

void surge::gl_atom::sprite_database::add_view(database sdb, GLuint64 texture_handle,
                                               glm::mat4 model_matrix, glm::vec4 image_view,
                                               glm::vec2 img_dims, float alpha) noexcept {
  using std::memcpy;

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::sprite::add_view");
#endif

  if (sdb->write_idx < sdb->max_sprites) {
    const auto u0{image_view[0]};
    const auto v0{image_view[1]};

    const auto w{image_view[2]};
    const auto h{image_view[3]};

    const auto W{img_dims[0]};
    const auto H{img_dims[1]};

    const glm::vec4 view_data{w / W, h / H, u0 / W, 1.0f - (v0 + h) / H};

    sprite_info si{};
    si.texture_handle = texture_handle;
    si.alpha = alpha;
    memcpy(si.model, glm::value_ptr(model_matrix), 16 * sizeof(float));
    memcpy(si.view, glm::value_ptr(view_data), 4 * sizeof(float));

    const auto idx{sdb->write_buffer * sdb->max_sprites + sdb->write_idx};
    sdb->buffer_data[idx] = si;
    sdb->write_idx++;
  } else {
    log_warn("Sprite database {} capacity exceeded. Ignoring push request",
             static_cast<void *>(sdb));
  }
}

void surge::gl_atom::sprite_database::add_view(database sdb, GLuint64 handle, glm::vec2 &&pos,
                                               glm::vec2 &&scale, float z, glm::vec4 image_view,
                                               glm::vec2 img_dims, float alpha) noexcept {
  const auto mv{glm::vec3{std::move(pos), z}};
  const auto sc{glm::vec3{std::move(scale), 1.0f}};
  const auto model{glm::scale(glm::translate(glm::mat4{1.0f}, mv), sc)};
  add_view(sdb, handle, model, image_view, img_dims, alpha);
}

void surge::gl_atom::sprite_database::add_view(database sdb, GLuint64 handle, const glm::vec2 &pos,
                                               const glm::vec2 &scale, float z,
                                               glm::vec4 image_view, glm::vec2 img_dims,
                                               float alpha) noexcept {
  const auto mv{glm::vec3{pos, z}};
  const auto sc{glm::vec3{scale, 1.0f}};
  const auto model{glm::scale(glm::translate(glm::mat4{1.0f}, mv), sc)};
  add_view(sdb, handle, model, image_view, img_dims, alpha);
}

void surge::gl_atom::sprite_database::add_depth(database sdb, GLuint64 texture, GLuint64 depth_map,
                                                glm::mat4 model) noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::sprite::add_depth");
#endif
  sdb->depth_texture_handle = texture;
  sdb->depth_map_handle = depth_map;
  sdb->depth_model = model;
}

void surge::gl_atom::sprite_database::draw(database sdb) noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::sprite::draw");
  TracyGpuZone("GPU surge::gl_atom::sprite::draw");
#endif

  // Regular sprites
  if (sdb->write_idx != 0) {
    glUseProgram(sdb->sprite_shader);

    const auto buffer_size{static_cast<GLsizeiptr>(sizeof(sprite_info) * sdb->write_idx)};
    const auto buffer_offset{
        static_cast<GLintptr>(sizeof(sprite_info) * sdb->write_buffer * sdb->max_sprites)};
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 3, sdb->buffer_id, buffer_offset, buffer_size);

    glBindVertexArray(sdb->VAO);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr,
                            gsl::narrow_cast<GLsizei>(sdb->write_idx));

    lock_and_advance_buffer(sdb, sdb->write_buffer);
  }

  // Depth sprite
  if (sdb->depth_texture_handle != 0 && sdb->depth_map_handle != 0) {
    glUseProgram(sdb->deep_sprite_shader);

    glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(sdb->depth_model));
    glUniformHandleui64ARB(4, sdb->depth_texture_handle);
    glUniformHandleui64ARB(5, sdb->depth_map_handle);

    glBindVertexArray(sdb->VAO);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, 1);
  }
}
