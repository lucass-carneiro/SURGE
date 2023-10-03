#include "globals.hpp"

#include <cstddef>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <gsl/gsl-lite.hpp>

namespace surge::mod::lasers::globals {

static glm::mat4 projection{1.0f};
static const glm::mat4 view{glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f))};

static surge::renderer::line::context line_ctx{};

} // namespace surge::mod::lasers::globals

void surge::mod::lasers::globals::make_projection(float ww, float wh) noexcept {
  projection = glm::ortho(0.0f, ww, wh, 0.0f);
}

auto surge::mod::lasers::globals::get_projection() noexcept -> const glm::mat4 & {
  return projection;
}

auto surge::mod::lasers::globals::get_view() noexcept -> const glm::mat4 & { return view; }

auto surge::mod::lasers::globals::make_line_ctx() noexcept -> std::uint32_t {
  using namespace surge::renderer::line;
  const auto ctx{create(glm::vec3{100.0f, 300.0f, 0.1}, glm::vec3{400.0f, 300.0f, 0.1})};
  if (ctx) {
    line_ctx = *ctx;
    return 0;
  } else {
    return static_cast<std::uint32_t>(ctx.error());
  }
}

auto surge::mod::lasers::globals::get_line_ctx() noexcept
    -> const surge::renderer::line::context & {
  return line_ctx;
}