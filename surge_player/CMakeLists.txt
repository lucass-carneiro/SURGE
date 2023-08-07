cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  SurgePlayer
  VERSION 1.0.0
  LANGUAGES CXX
)

# -----------------------------------------
# Option file
# -----------------------------------------

configure_file("${PROJECT_SOURCE_DIR}/include/options_in.txt" "${PROJECT_SOURCE_DIR}/include/options.hpp")

# -----------------------------------------
#  Target sources
# -----------------------------------------

set(
  SURGE_PLAYER_HEADER_LIST
  "${PROJECT_SOURCE_DIR}/include/allocators.hpp"
  "${PROJECT_SOURCE_DIR}/include/cli.hpp"
  "${PROJECT_SOURCE_DIR}/include/files.hpp"
  "${PROJECT_SOURCE_DIR}/include/font_cache.hpp"
  "${PROJECT_SOURCE_DIR}/include/logo.hpp"
  "${PROJECT_SOURCE_DIR}/include/logging.hpp"
  "${PROJECT_SOURCE_DIR}/include/mimalloc_override_new_delete.hpp"
  "${PROJECT_SOURCE_DIR}/include/module_manager.hpp"
  "${PROJECT_SOURCE_DIR}/include/options.hpp"
  "${PROJECT_SOURCE_DIR}/include/renderer.hpp"
  "${PROJECT_SOURCE_DIR}/include/timers.hpp"
  "${PROJECT_SOURCE_DIR}/include/window.hpp"
)

set(
  SURGE_PLAYER_SOURCE_LIST
  "${PROJECT_SOURCE_DIR}/source/allocators.cpp"
  "${PROJECT_SOURCE_DIR}/source/cli.cpp"
  "${PROJECT_SOURCE_DIR}/source/files.cpp"
  "${PROJECT_SOURCE_DIR}/source/font_cache.cpp"
  "${PROJECT_SOURCE_DIR}/source/image.cpp"
  "${PROJECT_SOURCE_DIR}/source/main.cpp"
  "${PROJECT_SOURCE_DIR}/source/module_manager.cpp"
  "${PROJECT_SOURCE_DIR}/source/renderer.cpp"
  "${PROJECT_SOURCE_DIR}/source/shaders.cpp"
  "${PROJECT_SOURCE_DIR}/source/timers.cpp"
  "${PROJECT_SOURCE_DIR}/source/uniforms.cpp"
  "${PROJECT_SOURCE_DIR}/source/window.cpp"
)

# -----------------------------------------
# Executable engine target
# -----------------------------------------

add_executable(SurgePlayer ${SURGE_PLAYER_HEADER_LIST} ${SURGE_PLAYER_SOURCE_LIST})
target_compile_features(SurgePlayer PRIVATE cxx_std_20)
set_target_properties(SurgePlayer PROPERTIES OUTPUT_NAME "surge")
set_target_properties(SurgePlayer PROPERTIES ENABLE_EXPORTS ON)

target_include_directories(
  SurgePlayer PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

if(SURGE_ENABLE_SANITIZERS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayer PUBLIC -fsanitize=address,null,unreachable,undefined)
    target_link_options(SurgePlayer PUBLIC -fsanitize=address,null,unreachable,undefined)
  else()
    # TODO
  endif()
endif()

if(SURGE_ENABLE_OPTIMIZATIONS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayer PUBLIC -O2)
    target_link_options(SurgePlayer PUBLIC -O2)
  else()
    target_compile_options(SurgePlayer PUBLIC /O2)
  endif()
endif()

if(SURGE_ENABLE_TUNING)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayer PUBLIC -march=native -mtune=native)
    target_link_options(SurgePlayer PUBLIC -march=native -mtune=native)
  else()
    # TODO
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayer PUBLIC -flto)
    target_link_options(SurgePlayer PUBLIC -flto)
  else()
    # TODO
  endif()
endif()

if(SURGE_ENABLE_FAST_MATH)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayer PUBLIC -ffast-math)
    target_link_options(SurgePlayer PUBLIC -ffast-math)
  else()
    # TODO
  endif()
endif()

# Dependencies and main program build type must match. If building dependencies static-debug, then /MTd must be used.
# See the link bellow for further information
# https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2012/2kzt1wy3(v=vs.110)?redirectedfrom=MSDN

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Generating a Debug build system")

  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayer PUBLIC -O0 -g3 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer)
    target_link_options(SurgePlayer PUBLIC -O0 -g3 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer)
  else()
    target_compile_options(SurgePlayer PUBLIC /MP /MDd)
  endif()

endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "Generating a Release build system")

  if(SURGE_ENABLE_TRACY)
    find_package(Tracy CONFIG REQUIRED)
    target_link_libraries(SurgePlayer PRIVATE Tracy::TracyClient)
  endif()

  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayer PUBLIC -g3)
    target_link_options(SurgePlayer PUBLIC -g3)
  else()
    target_compile_options(SurgePlayer PUBLIC /MP /MD)
  endif()

endif()

# -----------------------------------------
# Link and build order dependencies
# -----------------------------------------

target_link_libraries(SurgePlayer PRIVATE
  mimalloc-static
  gsl::gsl-lite
  EASTL
  yaml-cpp
  glfw
  glm::glm
  OpenGL::GL
  glad::glad
  Freetype::Freetype
)