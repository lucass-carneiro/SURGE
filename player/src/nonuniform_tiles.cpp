#include "nonuniform_tiles.hpp"

#include "files.hpp"
#include "logging.hpp"

#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

auto surge::atom::nonuniform_tiles::create(const tile_structure &structure) noexcept
    -> tl::expected<buffer_data, error> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::nonuniform_tiles::create");
  TracyGpuZone("GPU surge::atom::nonuniform_tiles::create");
#endif

  /**************
   * Load Image *
   **************/
  auto image_data{files::load_image(structure.file)};
  if (!image_data) {
    return tl::unexpected{image_data.error()};
  }

  const int channels = 4;
  const int format{image_data->channels == channels ? GL_RGBA : GL_RGB};

  /***************
   * Gen texture *
   ***************/
  log_info("Creating nonuniform tile texture");

  GLuint texture_id{0};
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texture_id);

  // Warpping
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Filtering
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,
                  static_cast<GLenum>(structure.filtering));
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER,
                  static_cast<GLenum>(structure.filtering));

  // Allocating and transfering texture memory
  // Tiles are a vertical strip. Its is way easier to load them into texture arrays like this.
  // See https://stackoverflow.com/a/59261544

  // clang-format off
  glTexImage3D(
    GL_TEXTURE_2D_ARRAY,
    0,
    format,
    image_data->width,
    image_data->height / structure.num_tiles,
    structure.num_tiles,
    0,
    format,
    GL_UNSIGNED_BYTE,
    image_data->texels
  );
  // clang-format off

  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

  surge::files::free_image(*image_data);

  // Unbinding
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

  /***************
   * Gen Buffers *
   ***************/
  log_info("Creating nonuniform tiles buffers");

  GLuint VAO{0};
  GLuint VBO{0};
  GLuint EBO{0};

  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glGenVertexArrays(1, &VAO);

  /***************
   * Create quad *
   ***************/
  log_info("Creating nonuniform tiles base quad");

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

  return buffer_data{texture_id, VBO, EBO, VAO};
}

void surge::atom::nonuniform_tiles::draw(GLuint shader_program, const buffer_data &ctx,
                                         const draw_data &dctx) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::nonuniform_tiles::draw");
  TracyGpuZone("GPU surge::atom::nonuniform_tiles::draw");
#endif

  glUseProgram(shader_program);

  renderer::uniforms::set(shader_program, "projection", dctx.projection);
  renderer::uniforms::set(shader_program, "view", dctx.view);

  renderer::uniforms::set(shader_program, "models", dctx.models.data(),
                          dctx.models.size());

  renderer::uniforms::set(shader_program, "texture_sampler", GLint{0});

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D_ARRAY, ctx.texture_id);

  glBindVertexArray(ctx.VAO);
  glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, dctx.models.size());
}

void surge::atom::nonuniform_tiles::cleanup(buffer_data &ctx) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::nonuniform_tiles::cleanup");
  TracyGpuZone("GPU surge::atom::nonuniform_tiles::cleanup");
#endif
  log_info("Deleting image buffer data (%u, %u, %u, %u)", ctx.VBO, ctx.EBO, ctx.texture_id,
           ctx.VAO);

  glDeleteBuffers(1, &(ctx.VBO));
  glDeleteBuffers(1, &(ctx.EBO));
  glDeleteTextures(1, &(ctx.texture_id));
  glDeleteVertexArrays(1, &(ctx.VAO));
}