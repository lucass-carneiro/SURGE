#ifndef SURGE_STATIC_MESH_HPP
#define SURGE_STATIC_MESH_HPP

#include "opengl/headers.hpp"

#include <array>
#include <concepts>

namespace surge {

enum class texture_type { diffuse, specular };

/**
 * @brief A statically sized single drawable mesh of triangles
 *
 * @tparam T The type used for positional data (floating point)
 * @tparam num_triangles The number of triangles in the mesh
 * @tparam num_textures The number of textures in the mesh
 */
template <std::floating_point T, std::size_t num_vertices, std::size_t num_triangles,
          std::size_t num_textures>
struct static_mesh {
  static constexpr const std::size_t cell_size{5};

  /**
   * @brief Vertex attributes are interleaved, that is, the attributes are a 1D array of repeating
   * cells. Each cell has the format:
   *
   * pos_x pos_y pos_z | tex_u tex_v | norm_x norm_y norm_z |
   *
   * See: https://stackoverflow.com/a/39684775
   *
   */
  std::array<T, cell_size * num_vertices> vertex_attributes{};

  std::array<GLuint, std::size_t{3} * num_triangles> draw_indices{};

  std::array<GLuint, num_textures> texture_id{};
  std::array<texture_type, num_textures> texture_types{};
};

/**
 * @brief Sends a static mesh and it's associated texture data to the passed GPU buffers
 *
 * @tparam T The type of the vertex data
 * @tparam num_triangles Number of triangles in the mesh
 * @tparam num_textures Number of textures in the mes
 * @param VAO The Vertex Array Object that holds the mesh
 * @param VBO The vertex buffer object that holds the vertex data.
 * @param EBO The element buffer object that holds the element data
 * @param mesh The mesh object.
 */
template <std::floating_point T, std::size_t num_vertices, std::size_t num_triangles,
          std::size_t num_textures>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void send_to_gpu(GLuint VAO, GLuint VBO, GLuint EBO,
                 const static_mesh<T, num_vertices, num_triangles, num_textures> &mesh) noexcept {
  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, mesh.cell_size * num_vertices * sizeof(T),
               mesh.vertex_attributes.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, std::size_t{3} * num_triangles * sizeof(GLuint),
               mesh.draw_indices.data(), GL_STATIC_DRAW);

  // vertex positions
  if constexpr (std::is_same<T, GLfloat>::value) {
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, mesh.cell_size * sizeof(T), nullptr);
  } else {
    glVertexAttribPointer(0, 3, GL_DOUBLE, GL_FALSE, mesh.cell_size * sizeof(T), nullptr);
  }
  glEnableVertexAttribArray(0);

  // vertex texture coords
  if constexpr (std::is_same<T, GLfloat>::value) {
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, mesh.cell_size * sizeof(T),
                          reinterpret_cast<GLvoid *>(3 * sizeof(T)));
  } else {
    glVertexAttribPointer(1, 2, GL_DOUBLE, GL_FALSE, mesh.cell_size * sizeof(T),
                          reinterpret_cast<GLvoid *>(3 * sizeof(T)));
  }
  glEnableVertexAttribArray(1);

  // vertex normals
  // if constexpr (std::is_same<T, GLfloat>::value) {
  //   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, mesh.cell_size * sizeof(T),
  //                         reinterpret_cast<GLvoid *>(3 * sizeof(T)));
  // } else {
  //   glVertexAttribPointer(1, 3, GL_DOUBLE, GL_FALSE, mesh.cell_size * sizeof(T),
  //                         reinterpret_cast<GLvoid *>(3 * sizeof(T)));
  // }
  // glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

/**
 * @brief Draws a static mesh
 *
 * @tparam T The type of the vertex data
 * @tparam num_triangles The number of triangles in the mesh
 * @tparam num_textures The number of textures in the mesh
 * @param VAO The vertex array object that holds the mesh data
 * @param mesh The mesh.
 */
template <std::floating_point T, std::size_t num_vertices, std::size_t num_triangles,
          std::size_t num_textures>
void draw(GLuint VAO, const static_mesh<T, num_vertices, num_triangles, num_textures> &) noexcept {
  // TODO: activate textures
  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, std::size_t{3} * num_triangles, GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);
}

} // namespace surge

#endif // SURGE_STATIC_MESH_HPP