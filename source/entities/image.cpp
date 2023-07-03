#include "entities/image.hpp"

#include "image_loader.hpp"
#include "logging_system/logging_system.hpp"
#include "window.hpp"

// clang-format off
#include "opengl/buffer_usage_hints.hpp"
#include "opengl/load_texture.hpp"
#include "opengl/uniforms.hpp"
// clang-format on

template <std::size_t i, typename T> [[nodiscard]] static inline auto buffer_offset() noexcept
    -> const void * {
  // NOLINTNEXTLINE
  return reinterpret_cast<const void *>(i * sizeof(T));
}

auto surge::image_entity::gen_buff() const noexcept -> GLuint {
  GLuint tmp{0};
  glGenBuffers(1, &tmp);
  return tmp;
}

auto surge::image_entity::gen_vao() const noexcept -> GLuint {
  GLuint tmp{0};
  glGenVertexArrays(1, &tmp);
  return tmp;
}

auto surge::image_entity::load_texture(const char *p, const char *ext) const noexcept
    -> texture_data {
  // When passing images to OpenGL they must be flipped.
  stbi_set_flip_vertically_on_load(static_cast<int>(true));

  auto image{load_image(p, ext)};
  if (!image) {
    stbi_set_flip_vertically_on_load(static_cast<int>(false));
    log_error("Unable to load spritesheet image file {}", p);
    return {};
  }

  stbi_set_flip_vertically_on_load(static_cast<int>(false));

  GLuint texture{0};
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  // Warpping
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // Filtering. TODO: Set via configs ?
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  const int format{image->channels_in_file == 4 ? GL_RGBA : GL_RGB};

  glTexImage2D(GL_TEXTURE_2D, 0, format, image->width, image->height, 0, format, GL_UNSIGNED_BYTE,
               image->data);
  glGenerateMipmap(GL_TEXTURE_2D);

  texture_data td{glm::vec2{image->width, image->height}, texture,
                  glm::vec2{1.0f / image->width, 1.0f / image->height}};
  stbi_image_free(image->data);

  return td;
}

void surge::image_entity::reset_geometry(const glm::vec3 &position,
                                         const glm::vec3 &scale) noexcept {

  current_quad.dims = scale;
  current_quad.corner = position;

  model_matrix = glm::mat4{1.0f};
  model_matrix = glm::translate(model_matrix, current_quad.corner);
  model_matrix = glm::scale(model_matrix, current_quad.dims);

  set_uniform(global_engine_window::get().get_sprite_shader(), "model", model_matrix);
}

void surge::image_entity::reset_geometry(glm::vec3 &&position, glm::vec3 &&scale) noexcept {
  reset_geometry(position, scale);
}

void surge::image_entity::reset_position(const glm::vec3 &position) noexcept {
  current_quad.corner = position;

  model_matrix = glm::mat4{1.0f};
  model_matrix = glm::translate(model_matrix, current_quad.corner);
  model_matrix = glm::scale(model_matrix, current_quad.dims);

  set_uniform(global_engine_window::get().get_sprite_shader(), "model", model_matrix);
}

void surge::image_entity::reset_position(glm::vec3 &&position) noexcept {
  reset_position(position);
}

void surge::image_entity::create_quad() noexcept {
  const std::array<float, 20> vertex_attributes{
      0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // bottom left
      1.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
      1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
      0.0f, 0.0f, 0.0f, 0.0f, 1.0f, // top left
  };

  const std::array<GLuint, 6> draw_indices{0, 1, 2, 2, 3, 0};

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertex_attributes.size() * sizeof(float), vertex_attributes.data(),
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, draw_indices.size() * sizeof(GLuint), draw_indices.data(),
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  // NOLINTNEXTLINE
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), buffer_offset<3, float>());
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void surge::image_entity::toggle_h_flip() noexcept { current_h_flip = !current_h_flip; }

void surge::image_entity::toggle_v_flip() noexcept { current_v_flip = !current_v_flip; }

void surge::image_entity::draw() noexcept {
  const auto &shader_program{global_engine_window::get().get_image_shader()};

  set_uniform(shader_program, "txt_0", GLint{0});
  set_uniform(shader_program, "model", model_matrix);
  set_uniform(shader_program, "ds", texture.ds);
  set_uniform(shader_program, "r0", glm::vec2{0.0f, 0.0f});
  set_uniform(shader_program, "dims", texture.dimentions);

  set_uniform(global_engine_window::get().get_image_shader(), "h_flip", current_h_flip);
  set_uniform(global_engine_window::get().get_image_shader(), "v_flip", current_v_flip);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture.gl_texture_idx);

  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

  glBindVertexArray(0);
}

void surge::image_entity::draw_region(glm::vec2 &&origin, glm::vec2 &&dims) noexcept {
  const auto &shader_program{global_engine_window::get().get_image_shader()};

  set_uniform(shader_program, "txt_0", GLint{0});
  set_uniform(shader_program, "model", model_matrix);
  set_uniform(shader_program, "ds", texture.ds);
  set_uniform(shader_program, "r0", origin);
  set_uniform(shader_program, "dims", dims);

  set_uniform(global_engine_window::get().get_image_shader(), "h_flip", current_h_flip);
  set_uniform(global_engine_window::get().get_image_shader(), "v_flip", current_v_flip);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture.gl_texture_idx);

  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

  glBindVertexArray(0);
}

void surge::image_entity::move(glm::vec3 &&vec) noexcept {
  current_quad.corner += vec;
  reset_position(current_quad.corner);
}

auto surge::image_entity::get_corner_coordinates() const noexcept -> glm::vec3 {
  return current_quad.corner;
}

surge::image_entity::image_entity(const char *sprite_set_path, glm::vec3 &&position,
                                  glm::vec3 &&scale, const char *sprite_sheet_ext) noexcept
    : VAO{gen_vao()},
      VBO{gen_buff()},
      EBO{gen_buff()},
      texture{load_texture(sprite_set_path, sprite_sheet_ext)} {

  create_quad();

  reset_geometry(std::forward<glm::vec3>(position), std::forward<glm::vec3>(scale));

  // Set initial flips to false
  set_uniform(global_engine_window::get().get_image_shader(), "v_flip", current_v_flip);
  set_uniform(global_engine_window::get().get_image_shader(), "h_flip", current_v_flip);
}