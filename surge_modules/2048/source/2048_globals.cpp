#include "2048_globals.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace mod_2048::globals {

static glm::mat4 projection{1.0f};
static const glm::mat4 view{glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                                        glm::vec3(0.0f, 1.0f, 0.0f))};

} // namespace mod_2048::globals

void mod_2048::globals::make_projection(float ww, float wh) noexcept {
  projection = glm::ortho(0.0f, ww, wh, 0.0f);
}

auto mod_2048::globals::get_projection() noexcept -> const glm::mat4 & { return projection; }

auto mod_2048::globals::get_view() noexcept -> const glm::mat4 & { return view; }