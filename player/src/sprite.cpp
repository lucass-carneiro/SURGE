#include "sprite.hpp"

#include "logging.hpp"
#include "renderer.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <gsl/gsl-lite.hpp>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

auto surge::atom::sprite::database::create(usize max_sprites) noexcept
    -> tl::expected<database, error> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::database::create");
  TracyGpuZone("GPU surge::atom::sprite::database::create");
#endif
  log_info("Creating sprite database");

  database db;

  /*******************
   * Compile shaders *
   *******************/

  const auto sprite_shader{
      renderer::create_shader_program("shaders/sprite.vert", "shaders/sprite.frag")};
  if (!sprite_shader) {
    log_error("Unable to create sprite shader");
    return tl::unexpected{sprite_shader.error()};
  }

  const auto deep_sprite_shader{
      renderer::create_shader_program("shaders/deep_sprite.vert", "shaders/deep_sprite.frag")};
  if (!deep_sprite_shader) {
    log_error("Unable to create deep sprite shader");
    return tl::unexpected{deep_sprite_shader.error()};
  }

  db.sprite_shader = *sprite_shader;
  db.deep_sprite_shader = *deep_sprite_shader;

  /***************
   * Gen Buffers *
   ***************/

  glCreateVertexArrays(1, &db.VAO);
  glCreateBuffers(1, &db.VBO);
  glCreateBuffers(1, &db.EBO);

  /***************
   * Create quad *
   ***************/
  const std::array<float, 20> vertex_attributes{
      0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // bottom left
      1.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
      1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
      0.0f, 0.0f, 0.0f, 0.0f, 1.0f, // top left
  };

  const std::array<GLuint, 6> draw_indices{0, 1, 2, 2, 3, 0};

  glBindVertexArray(db.VAO);

  glBindBuffer(GL_ARRAY_BUFFER, db.VBO);
  glBufferData(GL_ARRAY_BUFFER, vertex_attributes.size() * sizeof(float), vertex_attributes.data(),
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, db.EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, draw_indices.size() * sizeof(GLuint), draw_indices.data(),
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        reinterpret_cast<const void *>(3 * sizeof(float))); // NOLINT

  /***************
   * Create GBAs *
   ***************/
  db.texture_handles = gba<GLuint64>::create(max_sprites, "Sprite Texture Handles GBA");
  db.models = gba<glm::mat4>::create(max_sprites, "Sprite Modesl GBA");
  db.image_views = gba<glm::vec4>::create(max_sprites, "Sprite image views GBA");
  db.alphas = gba<float>::create(max_sprites, "Sprite Alphas GBA");

  return db;
}

void surge::atom::sprite::database::destroy() noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::database::destroy");
  TracyGpuZone("GPU surge::atom::sprite::database::destroy");
#endif
  log_info("Destroying sprite database");

  alphas.destroy();
  image_views.destroy();
  models.destroy();
  texture_handles.destroy();
  glDeleteBuffers(1, &(EBO));
  glDeleteBuffers(1, &(VBO));
  glDeleteVertexArrays(1, &(VAO));

  renderer::destroy_shader_program(sprite_shader);
  renderer::destroy_shader_program(deep_sprite_shader);
}

void surge::atom::sprite::database::add(GLuint64 handle, glm::mat4 model, float alpha) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::database::add");
#endif
  texture_handles.push(handle);
  models.push(model);
  image_views.push(glm::vec4{1.0f, 1.0f, 0.0f, 0.0f});
  alphas.push(alpha);
}

void surge::atom::sprite::database::add_depth(GLuint64 texture, GLuint64 depth_map,
                                              glm::mat4 model) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::database::add_depth");
#endif
  depth_texture_handle = texture;
  depth_map_handle = depth_map;
  depth_model = model;
}

void surge::atom::sprite::database::add_view(GLuint64 handle, glm::mat4 model, glm::vec4 image_view,
                                             glm::vec2 img_dims, float alpha) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::database::add_view");
#endif
  const auto u0{image_view[0]};
  const auto v0{image_view[1]};

  const auto w{image_view[2]};
  const auto h{image_view[3]};

  const auto W{img_dims[0]};
  const auto H{img_dims[1]};

  const glm::vec4 view_data{w / W, h / H, u0 / W, 1.0f - (v0 + h) / H};

  texture_handles.push(handle);
  models.push(model);
  image_views.push(view_data);
  alphas.push(alpha);
}

void surge::atom::sprite::database::reset() noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::database::reset()");
#endif
  depth_texture_handle = 0;
  depth_map_handle = 0;
  depth_model = glm::mat4{1.0f};

  texture_handles.reset();
  models.reset();
  image_views.reset();
  alphas.reset();
}

void surge::atom::sprite::database::reinit() noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::database::reinit()");
#endif
  depth_texture_handle = 0;
  depth_map_handle = 0;
  depth_model = glm::mat4{1.0f};

  texture_handles.reinit();
  models.reinit();
  image_views.reinit();
  alphas.reinit();
}

void surge::atom::sprite::database::translate(usize idx, const glm::vec3 &dir) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::database::translate()");
#endif

  auto elm{models.get_elm_ptr(idx)};

#ifdef SURGE_BUILD_TYPE_Debug
  if (!elm) {
    log_warn("Unable to get element pointer for translating index %lu", idx);
    return;
  }
#endif

  *elm = glm::translate(*elm, dir);
}

auto surge::atom::sprite::database::get_pos(usize idx) noexcept -> glm::vec3 {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::database::get_pos()");
#endif

  auto elm{models.get_elm_ptr(idx)};

#ifdef SURGE_BUILD_TYPE_Debug
  if (!elm) {
    log_warn("Unable to get element pointer for recovering the position index %lu", idx);
    return glm::vec3{0.0f};
  }
#endif

  return glm::vec3{(*elm)[3][0], (*elm)[3][1], (*elm)[3][2]};
}

void surge::atom::sprite::database::draw() noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::database::draw");
  TracyGpuZone("GPU surge::atom::sprite::database::draw");
#endif

  // Regular sprites
  if (texture_handles.size() != 0 && models.size() != 0 && alphas.size() != 0
      && image_views.size() != 0) {

    models.bind(GL_SHADER_STORAGE_BUFFER, 3);
    alphas.bind(GL_SHADER_STORAGE_BUFFER, 4);
    texture_handles.bind(GL_SHADER_STORAGE_BUFFER, 5);
    image_views.bind(GL_SHADER_STORAGE_BUFFER, 6);

    glUseProgram(sprite_shader);

    glBindVertexArray(VAO);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr,
                            gsl::narrow_cast<GLsizei>(models.size()));

    texture_handles.lock_write_buffer();
    models.lock_write_buffer();
    image_views.lock_write_buffer();
    alphas.lock_write_buffer();
  }

  // Depth sprite
  if (depth_texture_handle != 0 && depth_map_handle != 0) {
    glUseProgram(deep_sprite_shader);

    renderer::uniforms::set(3, depth_model);
    renderer::uniforms::set(4, depth_texture_handle);
    renderer::uniforms::set(5, depth_map_handle);

    glBindVertexArray(VAO);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, 1);
  }
}

auto surge::atom::sprite::place(glm::vec2 &&pos, glm::vec2 &&scale, float z) noexcept -> glm::mat4 {
  const auto mv{glm::vec3{std::move(pos), z}};
  const auto sc{glm::vec3{std::move(scale), 1.0f}};
  return glm::scale(glm::translate(glm::mat4{1.0f}, mv), sc);
}

auto surge::atom::sprite::place(const glm::vec2 &pos, const glm::vec2 &scale, float z) noexcept
    -> glm::mat4 {
  const auto mv{glm::vec3{pos, z}};
  const auto sc{glm::vec3{scale, 1.0f}};
  return glm::scale(glm::translate(glm::mat4{1.0f}, mv), sc);
}