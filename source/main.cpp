#include "allocators.hpp"
#include "cli.hpp"
#include "global_allocators.hpp"
#include "image_loader.hpp"
#include "log.hpp"
#include "opengl_buffer_pools.hpp"
#include "safe_ops.hpp"
#include "shader.hpp"
#include "squirrel_bindings.hpp"
#include "window.hpp"

#include <cstddef>
#include <cstdlib>
#include <exception>

const std::filesystem::path surge::global_file_log_manager::file_path =
    std::filesystem::path{"log.txt"};

const SQInteger surge::global_squirrel_vm::stack_size = 1024 * SQInteger{10};

const std::size_t surge::global_linear_arena_allocator::capacity = 16384;

const std::size_t surge::global_engine_window::subsystem_allocator_capacity =
    100;

const std::size_t surge::global_image_loader::subsystem_allocator_capacity =
    16084;
const std::size_t surge::global_image_loader::persistent_allocator_capacity =
    8042;
const std::size_t surge::global_image_loader::volatile_allocator_capacity = 100;

auto main(int argc, char **argv) noexcept -> int {
  using namespace surge;

  // Init log subsystem
  global_stdout_log_manager::get();
  global_file_log_manager::get();
  draw_logo();

  // Command line argument parsing
  const auto cmd_line_args = parse_arguments(argc, argv);
  if (!cmd_line_args) {
    return EXIT_FAILURE;
  }

  // Init remaining subsystems
  global_default_allocator::get();
  global_linear_arena_allocator::get();

  global_squirrel_vm::get();
  global_engine_window::get();

  global_image_loader::get();

  // Config script validation and engine context loading
  const auto valid_config_script =
      validate_config_script_path(*(cmd_line_args));
  if (!valid_config_script) {
    return EXIT_FAILURE;
  }

  const auto engine_context_pushed =
      global_squirrel_vm::get().load_context(*(valid_config_script));
  if (!engine_context_pushed) {
    return EXIT_FAILURE;
  }

  global_image_loader::get().load_persistent(
      "../resources/images/awesomeface.png");
  return EXIT_SUCCESS;

  // Initialize GLFW
  if (!global_engine_window::get().init()) {
    return EXIT_FAILURE;
  }

  // Compile and link shaders
  dynamic_shader default_vertex_shader(
      "../shaders/default.vert", GL_VERTEX_SHADER, "defualt_vertex_shader");
  dynamic_shader default_fragment_shader(
      "../shaders/default.frag", GL_FRAGMENT_SHADER, "defualt_fragment_shader");

  if (!default_vertex_shader.is_compiled() ||
      !default_fragment_shader.is_compiled()) {
    return EXIT_FAILURE;
  }

  shader_program default_program(default_vertex_shader, default_fragment_shader,
                                 "default_shader_program");
  if (!default_program.is_linked()) {
    return EXIT_FAILURE;
  }

  /*
  // Load triangle in memory
  std::array<float, 9> triangle_vertices{-0.5f, -0.5f, 0.0f, 0.5f, -0.5f,
                                         0.0f,  0.0f,  0.5f, 0.0f};
  static_vao_buffer_pool<1> VAOs;
  static_buffer_pool<1> BOs;

  VAOs.bind<0>();
  BOs.bind<0>(GL_ARRAY_BUFFER);

  BOs.transfer_data(GL_ARRAY_BUFFER, triangle_vertices.size() * sizeof(float),
                    triangle_vertices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  VAOs.enable(0);
  BOs.unbind(GL_ARRAY_BUFFER);
  VAOs.unbind();*/

  // Load quad in memory
  std::array<float, 12> quad_verticies{
      0.5f,  0.5f,  0.0f, // top right
      0.5f,  -0.5f, 0.0f, // bottom right
      -0.5f, -0.5f, 0.0f, // bottom left
      -0.5f, 0.5f,  0.0f  // top left
  };

  std::array<unsigned int, 6> quad_indices{
      // note that we start from 0!
      0, 1, 3, // first triangle
      1, 2, 3  // second triangle
  };

  static_vao_buffer_pool<1> VAOs;
  static_buffer_pool<2> BOs;

  VAOs.bind<0>();

  BOs.bind<0>(GL_ARRAY_BUFFER);
  BOs.transfer_data(GL_ARRAY_BUFFER, quad_verticies.size() * sizeof(float),
                    quad_verticies.data(), GL_STATIC_DRAW);

  BOs.bind<1>(GL_ELEMENT_ARRAY_BUFFER);
  BOs.transfer_data(GL_ELEMENT_ARRAY_BUFFER,
                    quad_indices.size() * sizeof(unsigned int),
                    quad_indices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  VAOs.enable(0);

  BOs.unbind(GL_ARRAY_BUFFER);
  // remember: do NOT unbind the EBO while a VAO is active as the bound element
  // buffer object IS stored in the VAO; keep the EBO bound.
  // BOs.unbind(GL_ELEMENT_ARRAY_BUFFER);

  VAOs.unbind();

  // Main loop

  while ((global_engine_window::get().frame_timer_reset_and_start(),
          !global_engine_window::get().should_close())) {

    // Handle events

    // Update states

    // Render calls
    glClearColor(GLfloat{global_engine_window::get().get_clear_color_r()},
                 GLfloat{global_engine_window::get().get_clear_color_g()},
                 GLfloat{global_engine_window::get().get_clear_color_b()},
                 GLfloat{global_engine_window::get().get_clear_color_a()});
    glClear(GL_COLOR_BUFFER_BIT);

    default_program.use();
    VAOs.bind<0>(); // seeing as we only have a single VAO there's
                    // no need to bind it every time, but we'll do
                    // so to keep things a bit more organized
    // glDrawArrays(GL_TRIANGLES, 0, 3); // draw triangle
    glDrawElements(GL_TRIANGLES, quad_indices.size(), GL_UNSIGNED_INT, nullptr);
    VAOs.unbind(); // no need to unbind it every time

    // Present rendering
    global_engine_window::get().swap_buffers();

    // Get events
    glfwPollEvents();

    // Compute elapsed time
    global_engine_window::get().frame_timmer_compute_dt();
  }

  // Normal shutdown
  default_program.destroy();

  return EXIT_SUCCESS;
}