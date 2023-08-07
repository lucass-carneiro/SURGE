#include "allocators.hpp"
#include "files.hpp"
#include "logging.hpp"
#include "renderer.hpp"

// clang-format off
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MALLOC(sz)           surge::allocators::mimalloc::malloc(sz)
#define STBI_REALLOC(p,newsz)     surge::allocators::mimalloc::realloc(p,newsz)
#define STBI_FREE(p)              surge::allocators::mimalloc::free(p)
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
// clang-format on

auto surge::renderer::image::create(const char *p) noexcept -> std::optional<context> {
  /**************
   * Load Image *
   **************/
  log_info("Loading image file %s", p);

  auto file{surge::files::load_file(p, false)};
  if (!file) {
    log_error("Unable to load image file %s", p);
    return {};
  }

  int iw{0}, ih{0}, channels_in_file{0};
  stbi_set_flip_vertically_on_load(static_cast<int>(true));
  auto img_data{stbi_load_from_memory(
      static_cast<stbi_uc *>(static_cast<void *>(file.value().data())),
      gsl::narrow_cast<int>(file.value().size()), &iw, &ih, &channels_in_file, 0)};
  stbi_set_flip_vertically_on_load(static_cast<int>(false));

  if (img_data == nullptr) {
    log_error("Unable to load image file %s due to stbi error: %s", p, stbi_failure_reason());
    surge::files::free_file(*file);
    return {};
  }

  surge::files::free_file(*file);

  /***************
   * Gen texture *
   ***************/
  log_info("Creating texture");

  GLuint texture_id{0};
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);

  // Warpping
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // Filtering
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Loading and mip mapping
  const int format{channels_in_file == 4 ? GL_RGBA : GL_RGB};

  glTexImage2D(GL_TEXTURE_2D, 0, format, iw, ih, 0, format, GL_UNSIGNED_BYTE, img_data);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(img_data);

  // Unbinding
  glBindTexture(GL_TEXTURE_2D, 0);

  /***************
   * Gen Buffers *
   ***************/
  log_info("Creating buffers");

  GLuint VAO{0};
  GLuint VBO{0};
  GLuint EBO{0};

  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glGenVertexArrays(1, &VAO);

  /***************
   * Create quad *
   ***************/
  log_info("Creating image quad");

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

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        reinterpret_cast<const void *>(3 * sizeof(float)));

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  /***************
   * Load shader *
   ***************/
  log_info("Creating shader");
  const auto img_shader{create_shader_program("shaders/image.vert", "shaders/image.frag")};
  if (!img_shader) {
    log_error("Unable to create image shader");
    return {};
  }

  return context{glm::vec2{iw, ih},
                 glm::vec2{1.0f / gsl::narrow_cast<float>(iw), 1.0f / gsl::narrow_cast<float>(ih)},
                 *img_shader,
                 texture_id,
                 VAO,
                 VBO,
                 EBO};
}

void surge::renderer::image::draw(context &ctx, draw_context &dctx) noexcept {

  const auto model{glm::scale(glm::translate(glm::mat4{1.0f}, dctx.pos), dctx.scale)};

  glUseProgram(ctx.shader_program);

  uniforms::set(ctx.shader_program, "projection", dctx.projection);
  uniforms::set(ctx.shader_program, "view", dctx.view);
  uniforms::set(ctx.shader_program, "model", model);

  uniforms::set(ctx.shader_program, "txt_0", GLint{0});
  uniforms::set(ctx.shader_program, "ds", ctx.ds);
  uniforms::set(ctx.shader_program, "r0", glm::vec2{0.0f, 0.0f});
  uniforms::set(ctx.shader_program, "dims", ctx.dimentions);

  // TODO: Implement
  uniforms::set(ctx.shader_program, "h_flip", dctx.h_flip);
  uniforms::set(ctx.shader_program, "v_flip", dctx.v_flip);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, ctx.texture_id);

  glBindVertexArray(ctx.VAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

  glBindVertexArray(0);
}

void surge::renderer::image::draw_region(context &ctx, draw_context &dctx, glm::vec2 &&origin,
                                         glm::vec2 &&dims) noexcept {

  const auto model{glm::scale(glm::translate(glm::mat4{1.0f}, dctx.pos), dctx.scale)};

  glUseProgram(ctx.shader_program);

  uniforms::set(ctx.shader_program, "projection", dctx.projection);
  uniforms::set(ctx.shader_program, "view", dctx.view);
  uniforms::set(ctx.shader_program, "model", model);

  uniforms::set(ctx.shader_program, "txt_0", GLint{0});
  uniforms::set(ctx.shader_program, "ds", ctx.ds);
  uniforms::set(ctx.shader_program, "r0", origin);
  uniforms::set(ctx.shader_program, "dims", dims);

  // TODO: Implement
  uniforms::set(ctx.shader_program, "h_flip", dctx.h_flip);
  uniforms::set(ctx.shader_program, "v_flip", dctx.v_flip);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, ctx.texture_id);

  glBindVertexArray(ctx.VAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

  glBindVertexArray(0);
}