#include "arena_allocator.hpp"
#include "cli.hpp"
#include "log.hpp"
#include "pool_allocator.hpp"
#include "safe_ops.hpp"
#include "squirrel_bindings.hpp"
#include "window.hpp"

#include "../shaders/default_frag.hpp"
#include "../shaders/default_vert.hpp"

#include <GL/gl.h>
#include <cstddef>
#include <cstdlib>
#include <exception>

const std::filesystem::path surge::global_file_log_manager::file_path =
    std::filesystem::path{"log.txt"};

const SQInteger surge::global_squirrel_vm::stack_size = 1024 * SQInteger{10};

const std::size_t surge::global_arena_allocator::arena_size =
    1024 * std::size_t{100};

inline void init_all_subsystems() noexcept {
  using namespace surge;
  global_stdout_log_manager::get();
  global_file_log_manager::get();
  global_squirrel_vm::get();
  global_arena_allocator::get();
  global_engine_window::get();
}

auto main(int argc, char **argv) noexcept -> int {
  using namespace surge;

  init_all_subsystems();

  // Command line argument parsing
  const auto cmd_line_args = parse_arguments(argc, argv);
  if (!cmd_line_args) {
    return EXIT_FAILURE;
  }

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

  // Initialize GLFW
  if (!global_engine_window::get().init()) {
    return EXIT_FAILURE;
  }

  // Compile and link shaders
  shader default_vertex_shader("default_vertex_shader", GL_VERTEX_SHADER,
                               shader_default_vert_src);
  shader default_fragment_shader("default_fragment_shader", GL_FRAGMENT_SHADER,
                                 shader_default_frag_src);

  default_vertex_shader.compile();
  default_fragment_shader.compile();

  if (!default_vertex_shader.is_valid() ||
      !default_fragment_shader.is_valid()) {
    glfwTerminate();
    return EXIT_FAILURE;
  }

  auto shader_program =
      link_shaders(default_vertex_shader, default_fragment_shader);
  if (!shader_program.has_value()) {
    glDeleteShader(default_vertex_shader.get_handle().value());
    glDeleteShader(default_fragment_shader.get_handle().value());
    glfwTerminate();
    return EXIT_FAILURE;
  }

  // triangle vertices
  std::array<float, 9> vertices{-0.5f, -0.5f, 0.0f, 0.5f, -0.5f,
                                0.0f,  0.0f,  0.5f, 0.0f};

  GLuint VBO{0}, VAO{0};
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  // bind the Vertex Array Object first, then bind and set vertex buffer(s), and
  // then configure vertex attributes(s).
  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), vertices.data(),
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  // note that this is allowed, the call to glVertexAttribPointer registered VBO
  // as the vertex attribute's bound vertex buffer object so afterwards we can
  // safely unbind
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // You can unbind the VAO afterwards so other VAO calls won't accidentally
  // modify this VAO, but this rarely happens. Modifying other VAOs requires a
  // call to glBindVertexArray anyways so we generally don't unbind VAOs (nor
  // VBOs) when it's not directly necessary.
  glBindVertexArray(0);

  // Main loop
  while (!global_engine_window::get().should_close()) {
    // Handle events

    // Update states

    // Render calls
    glClearColor(GLfloat{global_engine_window::get().get_clear_color_r()},
                 GLfloat{global_engine_window::get().get_clear_color_g()},
                 GLfloat{global_engine_window::get().get_clear_color_b()},
                 GLfloat{global_engine_window::get().get_clear_color_a()});
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program.value());
    glBindVertexArray(VAO); // seeing as we only have a single VAO there's
                            // no need to bind it every time, but we'll do
                            // so to keep things a bit more organized
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0); // no need to unbind it every time

    // Present rendering
    global_engine_window::get().swap_buffers();

    // Get events
    glfwPollEvents();
  }

  // Normal shutdown
  log_all<log_event::message>("Deleating shaders.");
  glDeleteShader(default_vertex_shader.get_handle().value());
  glDeleteShader(default_fragment_shader.get_handle().value());
  glDeleteProgram(shader_program.value());

  return EXIT_SUCCESS;
}