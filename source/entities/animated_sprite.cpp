#include "entities/animated_sprite.hpp"

#include "image_loader.hpp"
#include "log.hpp"
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

auto surge::animated_sprite::gen_buff() const noexcept -> GLuint {
  GLuint tmp{0};
  glGenBuffers(1, &tmp);
  return tmp;
}

auto surge::animated_sprite::gen_vao() const noexcept -> GLuint {
  GLuint tmp{0};
  glGenVertexArrays(1, &tmp);
  return tmp;
}

auto surge::animated_sprite::load_spriteset(const std::filesystem::path &p,
                                            const char *ext) const noexcept -> spriteset_data {
  // When passing images to OpenGL they must be flipped.
  stbi_set_flip_vertically_on_load(static_cast<int>(true));

  auto image{load_image(p, ext)};
  if (!image) {
    stbi_set_flip_vertically_on_load(static_cast<int>(false));
    log_error("Unable to load spritesheet image file {}", p.c_str());
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

  spriteset_data sd{glm::vec2{image->width, image->height}, texture};
  stbi_image_free(image->data);

  return sd;
}

void surge::animated_sprite::create_quad() noexcept {
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

void surge::animated_sprite::reset_geometry(const glm::vec3 &position,
                                            const glm::vec3 &scale) noexcept {

  if (sad_file.has_value()) {
    current_quad.dims = glm::vec3{
        static_cast<float>(sad_file->Sw[current_animation_data.animation_index]) * scale[0],
        static_cast<float>(sad_file->Sh[current_animation_data.animation_index]) * scale[1], 0.0};
  } else {
    current_quad.dims = glm::vec3{0.0, 0.0, 0.0};
    log_error("Unable to reset geometry. No sad file currently loaded");
  }

  current_quad.corner = position;

  model_matrix = glm::mat4{1.0f};
  model_matrix = glm::translate(model_matrix, current_quad.corner);
  model_matrix = glm::scale(model_matrix, current_quad.dims);

  set_uniform(global_engine_window::get().get_shader_program(), "model", model_matrix);
}

void surge::animated_sprite::reset_geometry(glm::vec3 &&position, glm::vec3 &&scale) noexcept {
  reset_geometry(position, scale);
}

void surge::animated_sprite::change_current_animation_to(std::uint32_t index, bool loops) noexcept {
  if (sad_file.has_value()) {
    if (index < sad_file->x.size()) {
      animation_data new_data{};
      new_data.animation_index = index;
      new_data.spritesheet_size = sad_file->rows[index] * sad_file->cols[index];
      new_data.loops = loops;
      new_data.h_flip = current_animation_data.h_flip;
      new_data.v_flip = current_animation_data.v_flip;
      current_animation_data = new_data;
    } else {
      log_error("Unable to recover animation index {}.", index);
    }
  } else {
    log_error("Unable to reset current animation. No sad file currently loaded");
  }
}

void surge::animated_sprite::toggle_h_flip() noexcept {
  current_animation_data.h_flip = !current_animation_data.h_flip;
}

void surge::animated_sprite::toggle_v_flip() noexcept {
  current_animation_data.v_flip = !current_animation_data.v_flip;
}

void surge::animated_sprite::draw() noexcept {
  const auto &shader_program{global_engine_window::get().get_shader_program()};

  set_uniform(shader_program, "txt_0", GLint{0});
  set_uniform(shader_program, "model", model_matrix);

  set_uniform(shader_program, "sheet_set_dimentions", spriteset.set_dimentions);

  set_uniform(global_engine_window::get().get_shader_program(), "h_flip",
              current_animation_data.h_flip);
  set_uniform(global_engine_window::get().get_shader_program(), "v_flip",
              current_animation_data.v_flip);

  if (sad_file.has_value()) {
    set_uniform(shader_program, "sheet_offsets",
                glm::vec2{sad_file->x[current_animation_data.animation_index],
                          sad_file->y[current_animation_data.animation_index]});

    set_uniform(shader_program, "sheet_dimentions",
                glm::vec2{sad_file->Sw[current_animation_data.animation_index],
                          sad_file->Sh[current_animation_data.animation_index]});
  } else {
    set_uniform(shader_program, "sheet_offsets", glm::vec2{0, 0});
    set_uniform(shader_program, "sheet_dimentions", glm::vec2{0, 0});
    log_error("Unable to set spritesheet offsets and dimentions in shader programs. "
              "No sad file currently loaded");
  }

  // TODO: Send linearized index to gpu
  set_uniform(shader_program, "sheet_indices", delinearize_animation_frame_index());

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, spriteset.gl_texture_idx);

  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

  glBindVertexArray(0);
}

void surge::animated_sprite::move(glm::vec3 &&vec) noexcept {
  current_quad.corner += vec;

  model_matrix = glm::translate(model_matrix, vec);
  set_uniform(global_engine_window::get().get_shader_program(), "model", model_matrix);
}

void surge::animated_sprite::scale(glm::vec3 &&vec) noexcept {
  const glm::vec3 position = current_quad.corner;

  current_quad.dims = current_quad.dims * vec;
  current_quad.corner = position;

  model_matrix = glm::mat4{1.0};
  model_matrix = glm::translate(model_matrix, current_quad.corner);
  model_matrix = glm::scale(model_matrix, current_quad.dims);
  set_uniform(global_engine_window::get().get_shader_program(), "model", model_matrix);
}

auto surge::animated_sprite::delinearize_animation_frame_index() const noexcept -> glm::vec2 {
  if (sad_file.has_value()) {
    const auto cols{sad_file->cols[current_animation_data.animation_index]};
    const auto index{current_animation_data.linearized_animation_frame_index};
    return glm::vec2{index / cols, index % cols};
  } else {
    return glm::vec2{0, 0};
  }
}

void surge::animated_sprite::update_animation_frame() noexcept {
  current_animation_data.linearized_animation_frame_index++;

  if (current_animation_data.loops) {
    current_animation_data.linearized_animation_frame_index
        %= current_animation_data.spritesheet_size;
  } else if (current_animation_data.linearized_animation_frame_index
             == current_animation_data.spritesheet_size) {
    current_animation_data.linearized_animation_frame_index--;
  }
}

void surge::animated_sprite::update(double frame_update_delay) noexcept {
  const auto dt{global_engine_window::get().get_frame_dt()};
  static double elapsed{dt};

  if (elapsed < frame_update_delay) {
    elapsed += dt;
  } else {
    elapsed = 0.0;
    update_animation_frame();
  }
}

surge::animated_sprite::animated_sprite(const std::filesystem::path &sprite_set_path,
                                        const std::filesystem::path &sad_file_path,
                                        std::uint32_t first_anim_idx, glm::vec3 &&position,
                                        glm::vec3 &&scale, const char *sprite_sheet_ext) noexcept
    : VAO{gen_vao()},
      VBO{gen_buff()},
      EBO{gen_buff()},
      spriteset{load_spriteset(sprite_set_path, sprite_sheet_ext)},
      sad_file{load_sad_file(sad_file_path)} {

  create_quad();

  change_current_animation_to(first_anim_idx);

  reset_geometry(std::forward<glm::vec3>(position), std::forward<glm::vec3>(scale));

  // Set initial flips to false
  set_uniform(global_engine_window::get().get_shader_program(), "v_flip",
              static_cast<GLboolean>(current_animation_data.v_flip));
  set_uniform(global_engine_window::get().get_shader_program(), "h_flip",
              static_cast<GLboolean>(current_animation_data.h_flip));
}