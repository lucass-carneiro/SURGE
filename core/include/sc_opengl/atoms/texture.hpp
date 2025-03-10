#ifndef SURGE_CORE_GL_ATOM_TEXTURE_HPP
#define SURGE_CORE_GL_ATOM_TEXTURE_HPP

#include "sc_container_types.hpp"
#include "sc_files.hpp"
#include "sc_integer_types.hpp"
#include "sc_logging.hpp"
#include "sc_opengl/sc_opengl.hpp"
#include "sc_options.hpp"

#include <optional>
#include <xxhash.h>

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

namespace surge::gl_atom::texture {

enum texture_filtering : GLint { nearest, linear, anisotropic };

enum class texture_wrap : GLint {
  repeat = GL_REPEAT,
  mirrored_repeat = GL_MIRRORED_REPEAT,
  clamp_to_edge = GL_CLAMP_TO_EDGE,
  clamp_to_border = GL_CLAMP_TO_BORDER
};

struct create_info {
  texture_filtering filtering{texture_filtering::linear};
  texture_wrap wrap{texture_wrap::clamp_to_edge};
  GLsizei mipmap_levels{4};
  bool make_resident{true};
};

struct create_data {
  GLuint id{0};
  GLuint64 handle{0};
  XXH64_hash_t name_hash{0};
};

using create_t = tl::expected<create_data, error>;
auto from_image(const create_info &ci, const files::image_data &img) noexcept -> create_t;
auto from_openEXR(const create_info &ci, const files::openEXR_image_data &img) noexcept -> create_t;
void destroy(create_data &cd) noexcept;
void destroy(GLuint id, GLuint64 handle) noexcept;

void make_resident(GLuint64 handle) noexcept;
void make_non_resident(GLuint64 handle) noexcept;

struct database {
private:
  vector<GLuint> ids;
  vector<GLuint64> handles;
  vector<XXH64_hash_t> name_hashes;

public:
  static auto create(usize initial_size) noexcept -> database;
  void destroy() noexcept;
  [[nodiscard]] auto find(const char *file_name) const noexcept -> std::optional<GLuint64>;

  void reset() noexcept;

  void add_openEXR(const create_info &ci, const char *path) noexcept;

  void add(const create_info &ci, std::convertible_to<std::string_view> auto &&...paths) noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
    ZoneScopedN("surge::gl_atom::texture::database::add");
    TracyGpuZone("GPU surge::gl_atom::texture::database::add");
#endif

    // Parallel load image files
    constexpr auto num_paths{sizeof...(paths)};
    std::array<files::img_future, num_paths> img_futures{};

    for (usize i = 0; const auto &p : std::initializer_list<const char *>{paths...}) {
      img_futures[i] = files::load_image_task(p);
      i++;
    }

    tasks::executor::get().wait_for_all();

    // Handle image load errors and push image data to record.
    for (auto &f : img_futures) {
      auto img{f.get()};
      if (img) {
        const auto texture_data{from_image(ci, *img)};
        if (texture_data) {
          ids.push_back(texture_data->id);
          handles.push_back(texture_data->handle);
          name_hashes.push_back(texture_data->name_hash);
        } else {
          log_error("Unable to create texture from %s", img->file_name);
        }
        files::free_image_task(*img);
      }
    }
  }

  auto add(const create_info &ci, const char *path) -> tl::expected<GLuint64, surge::error>;

  [[nodiscard]] inline auto size() const noexcept -> usize { return ids.size(); }

#ifdef SURGE_BUILD_TYPE_Debug
  [[nodiscard]] inline auto get_ids() const noexcept -> const vector<GLuint> & { return ids; }

  [[nodiscard]] inline auto get_handles() const noexcept -> const vector<GLuint64> & {
    return handles;
  }

  [[nodiscard]] inline auto get_hashes() const noexcept -> const vector<XXH64_hash_t> & {
    return name_hashes;
  }
#endif
};

} // namespace surge::gl_atom::texture

#endif // SURGE_CORE_GL_ATOM_TEXTURE_HPP