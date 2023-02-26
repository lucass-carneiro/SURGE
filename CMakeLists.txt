cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  Surge
  VERSION 1.0.0
  LANGUAGES CXX
)

# Generate compile_commands.json for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

# -----------------------------------------
# In-source build guard
# -----------------------------------------

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
    "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# -----------------------------------------
# External dependencies
# -----------------------------------------

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
list(APPEND VCPKG_FEATURE_FLAGS "versions")

include(FindOpenGL)
include(FindLuaJIT)

find_package(EASTL CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(docopt CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(gsl-lite CONFIG REQUIRED)
find_package(Taskflow CONFIG REQUIRED)
find_package(LuaJIT REQUIRED)
find_package(OpenGL REQUIRED)
find_package(mimalloc CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

# -----------------------------------------
# Size options
# -----------------------------------------

if(NOT DEFINED SURGE_STACK_FREE_RECORD_SIZE)
  set(SURGE_STACK_FREE_RECORD_SIZE 16)
elseif(SURGE_STACK_FREE_RECORD_SIZE LESS 16)
  message(WARNING "The variable SURGE_STACK_FREE_RECORD_SIZE should be set to at least 16. It will be automatically set to this value.")
  set(SURGE_STACK_FREE_RECORD_SIZE 16)
endif()

if(NOT DEFINED SURGE_OPENGL_ERROR_BUFFER_SIZE)
  set(SURGE_OPENGL_ERROR_BUFFER_SIZE 1024)
elseif(SURGE_OPENGL_ERROR_BUFFER_SIZE LESS 1024)
  message(WARNING "The variable SURGE_OPENGL_ERROR_BUFFER_SIZE should be set to at least 1024. It will be automatically set to this value.")
  set(SURGE_OPENGL_ERROR_BUFFER_SIZE 1024)
endif()

if(NOT DEFINED SURGE_FPS_COUNTER_SAMPLE_SIZE)
  set(SURGE_FPS_COUNTER_SAMPLE_SIZE 1000)
endif()

# -----------------------------------------
# Operating system and compiler detection
# -----------------------------------------

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(SURGE_SYSTEM_IS_POSIX "IS_POSIX")
else()
  set(SURGE_SYSTEM_IS_POSIX "NOT_POSIX")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
  set(SURGE_COMPILER_FLAG_STYLE "gcc")
else()
  set(SURGE_COMPILER_FLAG_STYLE "msvc")
endif()

# -----------------------------------------
# Logging options
# -----------------------------------------

option(SURGE_USE_LOG "Enable log messages" ON)
option(SURGE_USE_LOG_COLOR "Use colors on log outputs" ON)
option(SURGE_STBIMAGE_ERRORS "Enables more verbose error message strings in stb_image" ON)

# -----------------------------------------
# Compilation flag options
# -----------------------------------------

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  option(SURGE_ENABLE_SANITIZERS "Compiles code with sanitizers" ON)
  option(SURGE_ENABLE_OPTIMIZATIONS "Compiles code with optimizations" OFF)
  option(SURGE_ENABLE_LTO "Compiles code with link time optimizations" OFF)
  option(SURGE_ENABLE_FAST_MATH "Compiles code with fast math mode" OFF)
  option(SURGE_ENABLE_TUNING "Compiles code with architecture tuning" OFF)
option(SURGE_DEBUG_MEMORY "Enable custom allocators debug facilities" ON)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  option(SURGE_ENABLE_SANITIZERS "Compiles code with sanitizers" OFF)
  option(SURGE_ENABLE_OPTIMIZATIONS "Compiles code with optimizations" ON)
  option(SURGE_ENABLE_LTO "Compiles code with link time optimizations" ON)
  option(SURGE_ENABLE_FAST_MATH "Compiles code with fast math mode" ON)
  option(SURGE_ENABLE_TUNING "Compiles code with architecture tuning" ON)
  option(SURGE_DEBUG_MEMORY "Enable custom allocators debug facilities" OFF)
endif()

# -----------------------------------------
# Other options
# -----------------------------------------

option(SURGE_ENABLE_THREADS "Enables multithreading" ON)
option(SURGE_BUILD_TESTING "Build tests" ON)

# -----------------------------------------
# Option file
# -----------------------------------------

configure_file("${Surge_SOURCE_DIR}/include/options_in.txt" "${Surge_SOURCE_DIR}/include/options.hpp")

# -----------------------------------------
# Source files
# -----------------------------------------

set(
  SURGE_HEADER_LIST
  "${Surge_SOURCE_DIR}/include/allocators/allocator_utils.hpp"
  "${Surge_SOURCE_DIR}/include/allocators/allocators.hpp"
  "${Surge_SOURCE_DIR}/include/allocators/base_allocator.hpp"
  "${Surge_SOURCE_DIR}/include/allocators/default_allocator.hpp"
  "${Surge_SOURCE_DIR}/include/allocators/eastl_allocator.hpp"
  "${Surge_SOURCE_DIR}/include/allocators/global_allocators.hpp"
  "${Surge_SOURCE_DIR}/include/allocators/linear_arena_allocator.hpp"
  "${Surge_SOURCE_DIR}/include/allocators/stack_allocator.hpp"
  "${Surge_SOURCE_DIR}/include/allocators/static_arena_allocator.hpp"
  "${Surge_SOURCE_DIR}/include/allocators/stl_allocator.hpp"
  
  "${Surge_SOURCE_DIR}/include/gui_windows/gui_windows.hpp"
  
  "${Surge_SOURCE_DIR}/include/stb/stb_image.hpp"
  
  "${Surge_SOURCE_DIR}/include/opengl/buffer_usage_hints.hpp"
  "${Surge_SOURCE_DIR}/include/opengl/create_program.hpp"
  "${Surge_SOURCE_DIR}/include/opengl/gl_uniforms.hpp"
  "${Surge_SOURCE_DIR}/include/opengl/glm.hpp"
  "${Surge_SOURCE_DIR}/include/opengl/link_program.hpp"
  "${Surge_SOURCE_DIR}/include/opengl/load_and_compile_from_path.hpp"
  "${Surge_SOURCE_DIR}/include/opengl/load_and_compile_from_text.hpp"
  "${Surge_SOURCE_DIR}/include/opengl/load_texture.hpp"
  
  "${Surge_SOURCE_DIR}/include/lua/lua_bindings.hpp"
  "${Surge_SOURCE_DIR}/include/lua/lua_logs.hpp"
  "${Surge_SOURCE_DIR}/include/lua/lua_states.hpp"
  "${Surge_SOURCE_DIR}/include/lua/lua_utils.hpp"
  "${Surge_SOURCE_DIR}/include/lua/lua_vm.hpp"
  "${Surge_SOURCE_DIR}/include/lua/lua_wrappers.hpp"
  
  "${Surge_SOURCE_DIR}/include/entities/sprite.hpp"
  "${Surge_SOURCE_DIR}/include/entities/actor.hpp"
  
  "${Surge_SOURCE_DIR}/include/cli.hpp"
  "${Surge_SOURCE_DIR}/include/file.hpp"
  "${Surge_SOURCE_DIR}/include/image_loader.hpp"
  "${Surge_SOURCE_DIR}/include/log.hpp"
  "${Surge_SOURCE_DIR}/include/options.hpp"
  "${Surge_SOURCE_DIR}/include/sad_file.hpp"
  "${Surge_SOURCE_DIR}/include/safe_ops.hpp"
  "${Surge_SOURCE_DIR}/include/static_map.hpp"
  "${Surge_SOURCE_DIR}/include/task_executor.hpp"
  "${Surge_SOURCE_DIR}/include/thread_allocators.hpp"
  "${Surge_SOURCE_DIR}/include/window.hpp"
)

set(
  SURGE_SOURCE_LIST
  "${Surge_SOURCE_DIR}/source/allocators/default_allocator.cpp"
  "${Surge_SOURCE_DIR}/source/allocators/linear_arena_allocator.cpp"
  "${Surge_SOURCE_DIR}/source/allocators/stack_allocator.cpp"

  "${Surge_SOURCE_DIR}/source/gui_windows/main_gui_window.cpp"
  "${Surge_SOURCE_DIR}/source/gui_windows/memory_profiler_window.cpp"
  "${Surge_SOURCE_DIR}/source/gui_windows/fps_counter_window.cpp"
  
  "${Surge_SOURCE_DIR}/source/stb/stb_image_impl.cpp"
  
  "${Surge_SOURCE_DIR}/source/lua/lua_logs.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_states.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_utils.cpp"
  
  "${Surge_SOURCE_DIR}/source/lua/lua_actor_wrappers.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_callback_wrappers.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_sprite_wrappers.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_task_wrappers.cpp"

  "${Surge_SOURCE_DIR}/source/entities/sprite.cpp"
  "${Surge_SOURCE_DIR}/source/entities/actor.cpp"

  "${Surge_SOURCE_DIR}/source/cli.cpp"
  "${Surge_SOURCE_DIR}/source/file.cpp"
  "${Surge_SOURCE_DIR}/source/main.cpp"
  "${Surge_SOURCE_DIR}/source/thread_allocators.cpp"
  "${Surge_SOURCE_DIR}/source/window.cpp"
)

# -----------------------------------------
# Executable engine target
# -----------------------------------------

add_executable(Surge ${SURGE_HEADER_LIST} ${SURGE_SOURCE_LIST})
target_compile_features(Surge PRIVATE cxx_std_20)
set_target_properties(Surge PROPERTIES OUTPUT_NAME "surge")

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

target_compile_definitions(Surge PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLEW)

if(SURGE_ENABLE_SANITIZERS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge PUBLIC -fsanitize=address,null,unreachable,undefined)
    target_link_options(Surge PUBLIC -fsanitize=address,null,unreachable,undefined)
  else()
    # TODO
  endif()
endif()

if(SURGE_ENABLE_OPTIMIZATIONS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge PUBLIC -O2)
    target_link_options(Surge PUBLIC -O2)
  else()
    # TODO
  endif()
endif()

if(SURGE_ENABLE_TUNING)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge PUBLIC -march=native -mtune=native)
    target_link_options(Surge PUBLIC -march=native -mtune=native)
  else()
    # TODO
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge PUBLIC -flto -fuse-linker-plugin)
    target_link_options(Surge PUBLIC -flto -fuse-linker-plugin)
  else()
    # TODO
  endif()
endif()

if(SURGE_ENABLE_FAST_MATH)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge PUBLIC -ffast-math)
    target_link_options(Surge PUBLIC -ffast-math)
  else()
    # TODO
  endif()
endif()


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Generating a Debug build system")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge PUBLIC -O0 -g3 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer)
    target_link_options(Surge PUBLIC -O0 -g3 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer)
  else()
    # TODO:
  endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "Generating a Release build system")
endif()

# -----------------------------------------
# Link dependencies
# -----------------------------------------

target_link_libraries(Surge PRIVATE
  OpenGL::GL
  EASTL
  fmt::fmt-header-only
  docopt_s
  tl::expected
  glfw
  glm::glm
  gsl::gsl-lite
  Taskflow::Taskflow
  luajit-5.1
  mimalloc-static
  imgui::imgui
  implot::implot
  GTest::gtest
  glad::glad
)

target_include_directories(
  Surge PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# -----------------------------------------
# Compile tests
# -----------------------------------------

if(SURGE_BUILD_TESTING)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()
