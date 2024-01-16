#include "DTU.hpp"
#include "error_types.hpp"
#include "game_state.hpp"

// clang-format off
#include "integer_types.hpp"
#include "player/renderer.hpp"
#include "player/static_image.hpp"
#include "player/logging.hpp"

#include "main_menu/main_menu.hpp"
#include "static_image.hpp"
// clang-format on

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

// View / Projection

static glm::mat4 g_projection_matrix{1.0f};
static auto g_view_matrix{glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                                      glm::vec3(0.0f, 1.0f, 0.0f))};

auto DTU::get_projection() noexcept -> const glm::mat4 & { return g_projection_matrix; }
auto DTU::get_view() noexcept -> glm::mat4 & { return g_view_matrix; }

// Shaders
static GLuint g_img_shader{0};

auto DTU::get_img_shader() noexcept -> GLuint { return g_img_shader; }

// Image components
static DTU::vector<glm::vec2> g_static_image_dimentions{};
static DTU::vector<glm::vec2> g_static_image_ds{};
static DTU::vector<GLuint> g_static_image_texture_id{};
static DTU::vector<GLuint> g_static_image_VBO{};
static DTU::vector<GLuint> g_static_image_EBO{};
static DTU::vector<GLuint> g_static_image_VAO{};

static DTU::vector<glm::vec3> g_static_image_draw_noflip_pos{};
static DTU::vector<glm::vec3> g_static_image_draw_noflip_scale{};
static DTU::vector<glm::vec2> g_static_image_draw_noflip_region_origin{};
static DTU::vector<glm::vec2> g_static_image_draw_noflip_region_dims{};

auto DTU::components::static_image_buffer::dimentions() noexcept -> DTU::vector<glm::vec2> & {
  return g_static_image_dimentions;
}

auto DTU::components::static_image_buffer::ds() noexcept -> DTU::vector<glm::vec2> & {
  return g_static_image_ds;
}

auto DTU::components::static_image_buffer::texture_id() noexcept -> DTU::vector<GLuint> & {
  return g_static_image_texture_id;
}

auto DTU::components::static_image_buffer::VBO() noexcept -> DTU::vector<GLuint> & {
  return g_static_image_VBO;
}

auto DTU::components::static_image_buffer::EBO() noexcept -> DTU::vector<GLuint> & {
  return g_static_image_EBO;
}

auto DTU::components::static_image_buffer::VAO() noexcept -> DTU::vector<GLuint> & {
  return g_static_image_VAO;
}

void DTU::components::static_image_buffer::clean_and_reset() noexcept {
  for (std::size_t i = 0; i < g_static_image_dimentions.size(); i++) {
    surge::atom::static_image::one_buffer_data ctx{
        g_static_image_dimentions[i], g_static_image_ds[i],  g_static_image_texture_id[i],
        g_static_image_VBO[i],        g_static_image_EBO[i], g_static_image_VAO[i]};
    surge::atom::static_image::cleanup(ctx);
  }

  g_static_image_ds.clear();
  g_static_image_dimentions.clear();
  g_static_image_texture_id.clear();
  g_static_image_VBO.clear();
  g_static_image_EBO.clear();
  g_static_image_VAO.clear();
  g_static_image_draw_noflip_pos.clear();
  g_static_image_draw_noflip_scale.clear();
  g_static_image_draw_noflip_region_origin.clear();
  g_static_image_draw_noflip_region_dims.clear();
}

auto DTU::components::static_image_draw_noflip::pos() noexcept -> DTU::vector<glm::vec3> & {
  return g_static_image_draw_noflip_pos;
}

auto DTU::components::static_image_draw_noflip::scale() noexcept -> DTU::vector<glm::vec3> & {
  return g_static_image_draw_noflip_scale;
}

auto DTU::components::static_image_draw_noflip::region_origin() noexcept
    -> DTU::vector<glm::vec2> & {
  return g_static_image_draw_noflip_region_origin;
}

auto DTU::components::static_image_draw_noflip::region_dims() noexcept -> DTU::vector<glm::vec2> & {
  return g_static_image_draw_noflip_region_dims;
}

// States
static DTU::game_state g_front_state{0};
// static DTU::game_state g_back_state{0};
static DTU::game_state &g_current_state{g_front_state};

// On load
extern "C" SURGE_MODULE_EXPORT auto on_load(GLFWwindow *window) noexcept -> DTU::u32 {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::on_load");
#endif

  // Bind callbacks
  const auto bind_callback_stat{DTU::bind_callbacks(window)};
  if (bind_callback_stat != 0) {
    return bind_callback_stat;
  }

  // Initialize global 2D projection matrix
  const auto [ww, wh] = surge::window::get_dims(window);
  g_projection_matrix = glm::ortho(0.0f, ww, wh, 0.0f, -1.1f, 1.1f);

  // Load the static image shader
  const auto img_shader{
      surge::renderer::create_shader_program("shaders/image.vert", "shaders/image.frag")};
  if (!img_shader) {
    return static_cast<DTU::u32>(DTU::error::image_shader_creation);
  }
  g_img_shader = *img_shader;

  // Pre allocate memory for component lists
  constexpr const std::size_t base_component_list_size{8};
  g_static_image_ds.reserve(base_component_list_size);
  g_static_image_dimentions.reserve(base_component_list_size);
  g_static_image_texture_id.reserve(base_component_list_size);
  g_static_image_VBO.reserve(base_component_list_size);
  g_static_image_EBO.reserve(base_component_list_size);
  g_static_image_VAO.reserve(base_component_list_size);

  g_static_image_draw_noflip_pos.reserve(base_component_list_size);
  g_static_image_draw_noflip_scale.reserve(base_component_list_size);
  g_static_image_draw_noflip_region_origin.reserve(base_component_list_size);
  g_static_image_draw_noflip_region_dims.reserve(base_component_list_size);

  // Load first game state
  g_front_state
      = DTU::game_state{DTU::state::main_menu::state_load, DTU::state::main_menu::state_unload};

  const auto state_load_result{g_current_state.state_load()};
  if (state_load_result) {
    return static_cast<DTU::u32>(*state_load_result);
  }

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload(GLFWwindow *window) noexcept -> DTU::u32 {
  using namespace DTU;

  const auto unbind_callback_stat{unbind_callbacks(window)};
  if (unbind_callback_stat != 0) {
    return unbind_callback_stat;
  }

  const auto state_unload_result{g_current_state.state_unload()};
  if (state_unload_result) {
    return static_cast<DTU::u32>(*state_unload_result);
  }

  surge::renderer::cleanup_shader_program(g_img_shader);

  return 0;
}