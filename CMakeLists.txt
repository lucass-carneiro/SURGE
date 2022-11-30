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

include(FindOpenGL)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

# -----------------------------------------
# Options
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

option(SURGE_ENABLE_THREADS "Enables multithreading" ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  option(SURGE_USE_LOG "Enable log messages" ON)
  option(SURGE_USE_LOG_COLOR "Use colors on log outputs" ON)
  option(SURGE_BUILD_TESTING "Build tests" ON)
  option(SURGE_DEBUG_MEMORY "Enable custom allocators debug facilities" ON)
  option(SURGE_STBIMAGE_ERRORS "Enables more verbose error message strings in stb_image" ON)
else()
  option(SURGE_USE_LOG "Enable log messages" OFF)
  option(SURGE_USE_LOG_COLOR "Use colors on log outputs" OFF)
  option(SURGE_BUILD_TESTING "Build tests" OFF)
  option(SURGE_DEBUG_MEMORY "Enable custom allocators debug facilities" OFF)
  option(SURGE_STBIMAGE_ERRORS "Enables more verbose error message strings in stb_image" OFF)
endif()

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(SURGE_SYSTEM_IS_POSIX "IS_POSIX")
else()
  set(SURGE_SYSTEM_IS_POSIX "NOT_POSIX")
endif()

configure_file("${Surge_SOURCE_DIR}/include/options_in.txt" "${Surge_SOURCE_DIR}/include/options.hpp")

# -----------------------------------------
# Source files
# -----------------------------------------

# Core source files
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
  
  "${Surge_SOURCE_DIR}/include/imgui/imgui_impl_glfw.h"
  "${Surge_SOURCE_DIR}/include/imgui/imgui_impl_opengl3.h"
  "${Surge_SOURCE_DIR}/include/imgui/imgui_impl_opengl3_loader.h"
  
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
  
  "${Surge_SOURCE_DIR}/include/mesh/sprite.hpp"
  
  "${Surge_SOURCE_DIR}/include/cli.hpp"
  "${Surge_SOURCE_DIR}/include/file.hpp"
  "${Surge_SOURCE_DIR}/include/image_loader.hpp"
  "${Surge_SOURCE_DIR}/include/log.hpp"
  "${Surge_SOURCE_DIR}/include/safe_ops.hpp"
  "${Surge_SOURCE_DIR}/include/static_map.hpp"
  "${Surge_SOURCE_DIR}/include/task_executor.hpp"
  "${Surge_SOURCE_DIR}/include/thread_allocators.hpp"
  "${Surge_SOURCE_DIR}/include/vertex_renderer.hpp"
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

  "${Surge_SOURCE_DIR}/include/imgui/imgui_impl_glfw.cpp"
  "${Surge_SOURCE_DIR}/include/imgui/imgui_impl_opengl3.cpp"
  
  "${Surge_SOURCE_DIR}/source/stb/stb_image_impl.cpp"
  
  "${Surge_SOURCE_DIR}/source/lua/lua_logs.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_states.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_utils.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_wrappers.cpp"

  "${Surge_SOURCE_DIR}/source/mesh/sprite.cpp"

  "${Surge_SOURCE_DIR}/source/cli.cpp"
  "${Surge_SOURCE_DIR}/source/file.cpp"
  "${Surge_SOURCE_DIR}/source/main.cpp"
  "${Surge_SOURCE_DIR}/source/thread_allocators.cpp"
  "${Surge_SOURCE_DIR}/source/window.cpp"
)

# Executable target
add_executable(Surge ${SURGE_HEADER_LIST} ${SURGE_SOURCE_LIST})
target_compile_features(Surge PRIVATE cxx_std_20)
set_target_properties(Surge PROPERTIES OUTPUT_NAME "surge")

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

target_compile_definitions(Surge
  PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLEW
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Generating a debug build system")
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    target_compile_options(Surge PUBLIC  -g3 -O0 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer -fsanitize=address,null,unreachable,undefined)
    target_link_options(Surge PUBLIC -g3 -O0 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer -fsanitize=address,null,unreachable,undefined)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # TODO:
    target_compile_options(Surge PUBLIC /Wall /Wextra /permissive- /O2)
  endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "Generating a release build system")
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    target_compile_options(Surge PUBLIC -O2 -Wall -Wextra -pedantic)
    target_link_options(Surge PUBLIC -O2 -Wall -Wextra -pedantic)
  elseif(MSVC)
    # TODO:
    target_compile_options(Surge PUBLIC /O2)
  endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Profile")
  message(STATUS "Generating a profiling build system")
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    target_compile_options(Surge PUBLIC -g3 -O2 -pg)
    target_link_options(Surge PUBLIC -g3 -O2 -pg)
  elseif(MSVC)
    # TODO:
    target_compile_options(Surge PUBLIC /O2)
  endif()
endif()

# -----------------------------------------
# Link dependencies
# -----------------------------------------

target_link_libraries(Surge PRIVATE
  OpenGL::GL
  CONAN_PKG::glad
  CONAN_PKG::eastl
  CONAN_PKG::fmt
  CONAN_PKG::docopt.cpp
  CONAN_PKG::tl-expected
  CONAN_PKG::glfw
  CONAN_PKG::glm
  CONAN_PKG::gsl-lite
  CONAN_PKG::taskflow
  CONAN_PKG::luajit
  CONAN_PKG::mimalloc
  CONAN_PKG::imgui
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
