#ifndef SURGE_TEXTURE_HPP
#define SURGE_TEXTURE_HPP

#include "container_types.hpp"
#include "files.hpp"
#include "integer_types.hpp"
#include "logging.hpp"
#include "renderer.hpp"
#include "tasks.hpp"

#include <array>
#include <optional>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

namespace surge::atom::texture {

struct load_options {
  renderer::texture_filtering filtering{renderer::texture_filtering::linear};
  renderer::texture_wrap wrap{renderer::texture_wrap::clamp_to_edge};
  GLsizei mipmap_levels{4};
  bool make_resident{true};
};

class record {
public:
  record(usize initial_size);

  void load(const load_options &opts, std::convertible_to<std::string_view> auto &&...paths) {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
    ZoneScopedN("surge::atom::texture::record::load");
#endif

    // Parallel load image files
    using future_t = std::future<tl::expected<files::image, error>>;

    constexpr auto num_paths{sizeof...(paths)};
    std::array<future_t, num_paths> futures{};

    for (usize i = 0; const auto &p : std::initializer_list<const char *>{paths...}) {
      futures[i] = tasks::executor().async([=] { return files::load_image(p); });
      i++;
    }

    tasks::executor().wait_for_all();

    // Handle image load errors and push image data to record.
    for (auto &f : futures) {
      auto img{f.get()};
      if (img) {
        const auto texture{img_to_texture(opts, *img)};
        if (texture) {
          const auto [id, handle] = *texture;
          names.push_back(img->file_name);
          ids.push_back(id);
          handles.push_back(handle);
        } else {
          log_error("Unable to create texture from %s", img->file_name);
        }
        files::free_image(*img);
      }
    }

    // Make resident if configured to do so
    if (opts.make_resident) {
      reside_all_unchecked();
    }
  }

  void reside_all() noexcept;
  void unreside_all() noexcept;

  void unload_all() noexcept;

  auto find(const char *file_name) noexcept -> std::optional<GLuint64>;

  [[nodiscard]] inline auto size() const noexcept -> usize { return names.size(); }

private:
  using texture_result_t = tl::expected<std::tuple<GLuint, GLuint64>, error>;
  auto img_to_texture(const load_options &opts, const files::image &image) noexcept
      -> texture_result_t;

  void reside_all_unchecked() noexcept;

  vector<const char *> names;
  vector<GLuint> ids;
  vector<GLuint64> handles;
};

} // namespace surge::atom::texture

#endif // SURGE_TEXTURE_HPP