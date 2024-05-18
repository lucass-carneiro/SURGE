cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

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
  "${PROJECT_SOURCE_DIR}/include/container_types.hpp"
  "${PROJECT_SOURCE_DIR}/include/error_types.hpp"
  "${PROJECT_SOURCE_DIR}/include/files.hpp"
  "${PROJECT_SOURCE_DIR}/include/gl_includes.hpp"
  "${PROJECT_SOURCE_DIR}/include/gpu_bump_array.hpp"
  "${PROJECT_SOURCE_DIR}/include/integer_types.hpp"
  "${PROJECT_SOURCE_DIR}/include/logging.hpp"
  "${PROJECT_SOURCE_DIR}/include/module.hpp"
  "${PROJECT_SOURCE_DIR}/include/nonuniform_tiles.hpp"
  "${PROJECT_SOURCE_DIR}/include/options.hpp"
  "${PROJECT_SOURCE_DIR}/include/pv_ubo.hpp"
  "${PROJECT_SOURCE_DIR}/include/renderer.hpp"
  "${PROJECT_SOURCE_DIR}/include/sprite.hpp"
  "${PROJECT_SOURCE_DIR}/include/static_image.hpp"
  "${PROJECT_SOURCE_DIR}/include/static_mesh.hpp"
  "${PROJECT_SOURCE_DIR}/include/tasks.hpp"
  "${PROJECT_SOURCE_DIR}/include/text.hpp"
  "${PROJECT_SOURCE_DIR}/include/texture.hpp"
  "${PROJECT_SOURCE_DIR}/include/timers.hpp"
  "${PROJECT_SOURCE_DIR}/include/window.hpp"
)

set(
  SURGE_PLAYER_SOURCE_LIST
  "${PROJECT_SOURCE_DIR}/src/allocators.cpp"
  "${PROJECT_SOURCE_DIR}/src/cli.cpp"
  "${PROJECT_SOURCE_DIR}/src/config.cpp"
  "${PROJECT_SOURCE_DIR}/src/files.cpp"
  "${PROJECT_SOURCE_DIR}/src/glad_gl_impl.cpp"
  "${PROJECT_SOURCE_DIR}/src/main.cpp"
  "${PROJECT_SOURCE_DIR}/src/module.cpp"
  "${PROJECT_SOURCE_DIR}/src/nonuniform_tiles.cpp"
  "${PROJECT_SOURCE_DIR}/src/pv_ubo.cpp"
  "${PROJECT_SOURCE_DIR}/src/renderer.cpp"
  "${PROJECT_SOURCE_DIR}/src/shaders.cpp"
  "${PROJECT_SOURCE_DIR}/src/sprite.cpp"
  "${PROJECT_SOURCE_DIR}/src/static_image.cpp"
  "${PROJECT_SOURCE_DIR}/src/static_mesh.cpp"
  "${PROJECT_SOURCE_DIR}/src/tasks.cpp"
  "${PROJECT_SOURCE_DIR}/src/text.cpp"
  "${PROJECT_SOURCE_DIR}/src/texture.cpp"
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

# Use UTF-8 on MSVC
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgePlayer PUBLIC /utf-8)
endif()

# Disable min/max macros on msvc
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgePlayer PUBLIC /D NOMINMAX)
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
    message(WARNING "Sanitizers don't work on MSVC yet.")
  endif()
endif()

if(SURGE_ENABLE_OPTIMIZATIONS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayer PUBLIC -O3)
    target_link_options(SurgePlayer PUBLIC -O3)
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
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
      target_compile_options(
          SurgePlayer
          PUBLIC
          -Og
          -g3
          -ggdb3
          -fno-omit-frame-pointer
          -Werror
          -Wall
          -Wextra
          -Wpedantic
          -Walloca
          -Wcast-qual
          -Wformat=2
          -Wformat-security
          -Wnull-dereference
          -Wstack-protector
          -Wvla
          -Wconversion
          -Warray-bounds
          -Warray-bounds-pointer-arithmetic
          -Wconditional-uninitialized
          -Wimplicit-fallthrough
          -Wpointer-arith
          -Wformat-type-confusion
          -Wfloat-equal
          -Wassign-enum
          -Wtautological-constant-in-range-compare
          -Wswitch-enum
          -Wshift-sign-overflow
          -Wloop-analysis
      )
      target_link_options(SurgePlayer PUBLIC -Og -g3 -ggdb3 -rdynamic)
  else()
    target_compile_options(SurgePlayer PUBLIC /Wall /MP /MDd)
    target_link_options(SurgePlayer PUBLIC /DEBUG:FULL)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgePlayer PUBLIC debuginfod)
  endif()

endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    message(WARNING ${VCPKG_LIBRARY_LINKAGE})
    if(VCPKG_TARGET_TRIPLET MATCHES "x64-windows-static") #TODO: This is not general and static releases do not work
      target_compile_options(SurgePlayer PUBLIC /MP /MT)
    else()
      target_compile_options(SurgePlayer PUBLIC /MP /MD)
    endif()
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Profile")
  target_link_libraries(SurgePlayer PUBLIC Tracy::TracyClient)
  
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayer PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer)
    target_link_options(SurgePlayer PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer -rdynamic)
  else()
    target_compile_options(SurgePlayer PUBLIC /MP /MD)
    target_link_options(SurgePlayer PUBLIC /DEBUG:FULL)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgePlayer PUBLIC debuginfod)
    target_link_libraries(SurgePlayer PUBLIC unwind)
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
  unofficial::inih::libinih
  foonathan_memory
  glfw
  glm::glm
  OpenGL::GL
  imgui::imgui
  Freetype::Freetype
  xxHash::xxhash
  OpenEXR::OpenEXR
)
