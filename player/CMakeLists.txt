cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

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

configure_file(
  "${PROJECT_SOURCE_DIR}/include/options_in.txt"
  "${PROJECT_SOURCE_DIR}/include/options.hpp"
)

# -----------------------------------------
#  Target sources
# -----------------------------------------

set(
  SURGE_PLAYER_HEADER_LIST
  "${PROJECT_SOURCE_DIR}/include/allocators.hpp"
  "${PROJECT_SOURCE_DIR}/include/cli.hpp"
  "${PROJECT_SOURCE_DIR}/include/config.hpp"
  "${PROJECT_SOURCE_DIR}/include/files.hpp"
  "${PROJECT_SOURCE_DIR}/include/logging.hpp"
  "${PROJECT_SOURCE_DIR}/include/module.hpp"
  "${PROJECT_SOURCE_DIR}/include/options.hpp"
  "${PROJECT_SOURCE_DIR}/include/override_new_delete.hpp"
  "${PROJECT_SOURCE_DIR}/include/renderer.hpp"
  "${PROJECT_SOURCE_DIR}/include/static_image.hpp"
  "${PROJECT_SOURCE_DIR}/include/static_mesh.hpp"
  "${PROJECT_SOURCE_DIR}/include/timers.hpp"
  "${PROJECT_SOURCE_DIR}/include/window.hpp"
)

set(
  SURGE_PLAYER_SOURCE_LIST
  "${PROJECT_SOURCE_DIR}/src/allocators.cpp"
  "${PROJECT_SOURCE_DIR}/src/cli.cpp"
  "${PROJECT_SOURCE_DIR}/src/config.cpp"
  "${PROJECT_SOURCE_DIR}/src/files.cpp"
  "${PROJECT_SOURCE_DIR}/src/main.cpp"
  "${PROJECT_SOURCE_DIR}/src/module.cpp"
  "${PROJECT_SOURCE_DIR}/src/renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/shaders.cpp"
  "${PROJECT_SOURCE_DIR}/src/static_image.cpp"
  "${PROJECT_SOURCE_DIR}/src/static_mesh.cpp"
  "${PROJECT_SOURCE_DIR}/src/timers.cpp"
  "${PROJECT_SOURCE_DIR}/src/uniforms.cpp"
  "${PROJECT_SOURCE_DIR}/src/window.cpp"
)

# -----------------------------------------
# Executable engine target
# -----------------------------------------

add_executable(SurgePlayer ${SURGE_PLAYER_HEADER_LIST} ${SURGE_PLAYER_SOURCE_LIST})
target_compile_features(SurgePlayer PRIVATE cxx_std_20)
set_target_properties(SurgePlayer PROPERTIES OUTPUT_NAME "surge")
set_target_properties(SurgePlayer PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS True)
set_target_properties(SurgePlayer PROPERTIES ENABLE_EXPORTS True)

# Enables __VA_OPT__ on msvc
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgePlayer PUBLIC /Zc:preprocessor)
endif()

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
    message(WARNING "Sanitizers don't work on MSVC yet.")# because of some "annotations" bullshit
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
    message(WARNING "TODO: Unknow tuning flags for msvc")
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayer PUBLIC -flto)
    target_link_options(SurgePlayer PUBLIC -flto)
  else()
    message(WARNING "TODO: Unknow LTO flag for msvc")
  endif()
endif()

if(SURGE_ENABLE_FAST_MATH)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayer PUBLIC -ffast-math)
    target_link_options(SurgePlayer PUBLIC -ffast-math)
  else()
    target_compile_options(SurgePlayer PUBLIC /fp:fast)
  endif()
endif()

# Dependencies and main program build type must match.
# If building dependencies static-debug, then /MTd must be used.
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

target_link_libraries(SurgePlayer PUBLIC
  mimalloc
  mimalloc-static
  gsl::gsl-lite
  tl::expected
  yaml-cpp
  EASTL
  glfw
  glm::glm
  OpenGL::GL
  glad::glad
  imgui::imgui
)