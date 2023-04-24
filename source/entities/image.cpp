#include "entities/image.hpp"

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

auto surge::image_entity::load_texture(const std::filesystem::path &p,
                                       const char *ext) const noexcept -> texture_data {
  // When passing images to OpenGL they must be flipped.
  stbi_set_flip_vertically_on_load(static_cast<int>(true));

  auto image{load_image(p, ext)};
  if (!image) {
    stbi_set_flip_vertically_on_load(static_cast<int>(false));
#ifdef SURGE_SYSTEM_Windows
    log_error(L"Unable to load spritesheet image file {}", p.c_str());
#else
    log_error("Unable to load spritesheet image file {}", p.c_str());
#endif
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

  texture_data td{glm::vec2{image->width, image->height}, texture};
  stbi_image_free(image->data);

  return td;
}

void surge::image_entity::create_quad(glm::vec3 &position, glm::vec3 &scale) noexcept {
  const auto px{position[0]};
  const auto py{position[1]};
  const auto pz{position[2]};

  const auto tw{texture.dimentions[0] * scale[0]};
  const auto th{texture.dimentions[1] * scale[1]};

  const std::array<float, 20> vertex_attributes{
      px,      py + th, pz, 0.0f, 0.0f, // bottom left
      px + tw, py + th, pz, 1.0f, 0.0f, // bottom right
      px + tw, py,      pz, 1.0f, 1.0f, // top right
      px,      py,      pz, 0.0f, 1.0f, // top left
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

  set_uniform(global_engine_window::get().get_image_shader(), "h_flip", current_h_flip);
  set_uniform(global_engine_window::get().get_image_shader(), "v_flip", current_v_flip);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture.gl_texture_idx);

  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

  glBindVertexArray(0);
}

surge::image_entity::image_entity(const std::filesystem::path &sprite_set_path,
                                  glm::vec3 &&position, glm::vec3 &&scale,
                                  const char *sprite_sheet_ext) noexcept
    : VAO{gen_vao()},
      VBO{gen_buff()},
      EBO{gen_buff()},
      texture{load_texture(sprite_set_path, sprite_sheet_ext)} {

  create_quad(position, scale);

  // Set initial flips to false
  set_uniform(global_engine_window::get().get_image_shader(), "v_flip", current_v_flip);
  set_uniform(global_engine_window::get().get_image_shader(), "h_flip", current_v_flip);
}