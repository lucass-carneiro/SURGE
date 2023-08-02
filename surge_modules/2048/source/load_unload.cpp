#include "2048.hpp"
#include "allocators.hpp"
#include "files.hpp"
#include "logging.hpp"
#include "window.hpp"

#include <glm/gtc/matrix_transform.hpp>

bgfx::ProgramHandle game::img_program_handle{0};

extern "C" {

SURGE_MODULE_EXPORT auto on_load(GLFWwindow *window) noexcept -> bool {
  log_info("Loading 2048 module");

  log_info("Initializing view and projection matrices");
  bgfx::setViewName(0, "2048 main view");
  bgfx::setViewMode(0, bgfx::ViewMode::DepthAscending);

  // View matrix
  const auto eye{glm::vec3{0.0f, 0.0f, 1.0f}};
  const auto center{glm::vec3{0.0f, 0.0f, 0.0f}};
  const auto up{glm::vec3{0.0f, 1.0f, 1.0f}};
  const auto view_matrix{glm::lookAt(eye, center, up)};

  // Projection matrix
  const auto [ww, wh] = surge::window::get_dims(window);
  const auto projection_matrix{glm::ortho(0.0f, ww, wh, 0.0f, -1.0f, 1.0f)};
  bgfx::setViewTransform(0, static_cast<const void *>(&view_matrix),
                         static_cast<const void *>(&projection_matrix));

  // Shader loading
  log_info("Loading image shader files");
  auto img_vert{surge::files::load_file("./shaders/image.vert.bin", false)};
  if (!img_vert) {
    log_error("Unable to load shader file image.vert.bin");
    return false;
  }

  auto img_frag{surge::files::load_file("./shaders/image.frag.bin", false)};
  if (!img_frag) {
    log_error("Unable to load shader file image.frag.bin");
    return false;
  }

  auto img_vert_ref{bgfx::makeRef(img_vert->data(), img_vert->size(), [](void *data, void *) {
    surge::allocators::mimalloc::free(data);
  })};
  auto img_frag_ref{bgfx::makeRef(img_frag->data(), img_frag->size(), [](void *data, void *) {
    surge::allocators::mimalloc::free(data);
  })};

  auto img_vs_handle{bgfx::createShader(img_vert_ref)};
  bgfx::setName(img_vs_handle, "Image vertex shader");

  auto img_fs_handle{bgfx::createShader(img_frag_ref)};
  bgfx::setName(img_fs_handle, "Image fragment shader");

  game::img_program_handle = bgfx::createProgram(img_vs_handle, img_fs_handle, true);

  return true;
}

SURGE_MODULE_EXPORT void on_unload() noexcept {
  log_info("Unloading 2048 module");

  log_info("Destroying Image shader program");
  bgfx::destroy(game::img_program_handle);
}
}