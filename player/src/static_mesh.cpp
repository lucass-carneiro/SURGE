#include "static_mesh.hpp"

#include "allocators.hpp"
#include "logging.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <foonathan/memory/std_allocator.hpp>
#include <gsl/gsl-lite.hpp>
#include <tiny_obj_loader.h>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

auto surge::atom::static_mesh::gen_triangle() noexcept -> std::tuple<GLuint, std::size_t> {

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN(" surge::atom::static_mesh::gen_triangle");
  TracyGpuZone("GPU surge::atom::static_mesh::gen_triangle");
#endif

  // clang-format off
  arr<float, 9> vd{
    0.0, 1.0, 0.0,
    1.0, 1.0, 0.0,
    0.5, 0.5, 0.0
  };
  // clang-format on

  arr<GLuint, 6> ed{0, 1, 2};

  GLuint VAO{0};
  GLuint VBO{0};
  GLuint EBO{0};

  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glGenVertexArrays(1, &VAO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vd.size() * sizeof(float), vd.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, ed.size() * sizeof(GLuint), ed.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

  return std::make_tuple(VAO, ed.size());
}

auto surge::atom::static_mesh::gen_square() noexcept -> std::tuple<GLuint, std::size_t> {

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN(" surge::atom::static_mesh::gen_square");
  TracyGpuZone("GPU surge::atom::static_mesh::gen_square");
#endif

  // clang-format off
  arr<float, 12> vd{
    0.0, 0.0, 0.0,
    1.0, 0.0, 0.0,
    1.0, 1.0, 0.0,
    0.0, 1.0, 0.0
  };
  // clang-format on

  arr<GLuint, 6> ed{0, 1, 2, 2, 3, 0};

  GLuint VAO{0};
  GLuint VBO{0};
  GLuint EBO{0};

  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glGenVertexArrays(1, &VAO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vd.size() * sizeof(float), vd.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, ed.size() * sizeof(GLuint), ed.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

  return std::make_tuple(VAO, ed.size());
}

auto surge::atom::static_mesh::load(const char *path) noexcept
    -> tl::expected<one_buffer_data, files::file_error> {

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN(" surge::atom::static_mesh::load");
  TracyGpuZone("GPU surge::atom::static_mesh::load");
#endif

  log_info("Loading OBJ file %s", path);

  if (!files::validate_path(path)) {
    return tl::unexpected(files::file_error::invalid_path);
  }

  // Tiny OBJ Loader does not allow custom allocator containers :(
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path)) {
    if (warn.size() != 0) {
      log_warn("%s", warn.c_str());
    } else {
      log_error("%s", err.c_str());
    }
    return tl::unexpected(files::file_error::invalid_format);
  }

  const auto &shape{shapes[0]};
  const auto &vtx{attrib.vertices};

  vec<GLuint> idx(shape.mesh.indices.size());
  idx.reserve(shapes[0].mesh.indices.size());

  for (const auto &index : shape.mesh.indices) {
    idx.push_back(index.vertex_index);
  }

  GLuint VAO{0};
  GLuint VBO{0};
  GLuint EBO{0};

  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glGenVertexArrays(1, &VAO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(tinyobj::real_t), vtx.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(GLuint), idx.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

  return one_buffer_data{VAO, gsl::narrow_cast<GLsizei>(idx.size())};
}

void surge::atom::static_mesh::draw(GLuint shader_program, const one_buffer_data &bd,
                                    const one_draw_data &dd) noexcept {

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN(" surge::atom::static_mesh::draw");
  TracyGpuZone("GPU surge::atom::static_mesh::draw");
#endif

  using namespace surge::renderer;

  glUseProgram(shader_program);

  uniforms::set(shader_program, "projection", dd.projection);
  uniforms::set(shader_program, "view", dd.view);
  uniforms::set(shader_program, "model", dd.model);
  uniforms::set(shader_program, "color", dd.color);

  glBindVertexArray(bd.VAO);
  glDrawElements(GL_TRIANGLES, bd.elements, GL_UNSIGNED_INT, nullptr);

  glBindVertexArray(0);
}