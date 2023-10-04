#include "lasers.hpp"

extern "C" SURGE_MODULE_EXPORT auto draw() noexcept -> std::uint32_t {
  using namespace surge::mod::lasers;
  using namespace surge::renderer;

  smo::draw(smo::type::lines, get_global_cell_grid_ctx(),
            smo::draw_context{get_global_projection(), get_global_view(), glm::mat4{1.0f},
                              glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}});

  return 0;
}