cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  SurgeCore
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
  "${PROJECT_SOURCE_DIR}/include/gl_atoms/gba.hpp"
  "${PROJECT_SOURCE_DIR}/include/gl_atoms/pv_ubo.hpp"
  "${PROJECT_SOURCE_DIR}/include/gl_atoms/shaders.hpp"
  "${PROJECT_SOURCE_DIR}/include/gl_atoms/sprite.hpp"
  "${PROJECT_SOURCE_DIR}/include/gl_atoms/text.hpp"
  "${PROJECT_SOURCE_DIR}/include/gl_atoms/texture.hpp"
  
  "${PROJECT_SOURCE_DIR}/include/vk_bootstrap/VkBootstrap.hpp"
  "${PROJECT_SOURCE_DIR}/include/vk_bootstrap/VkBootstrapDispatch.hpp"
  
  "${PROJECT_SOURCE_DIR}/include/allocators.hpp"
  "${PROJECT_SOURCE_DIR}/include/cli.hpp"
  "${PROJECT_SOURCE_DIR}/include/config.hpp"
  "${PROJECT_SOURCE_DIR}/include/container_types.hpp"
  "${PROJECT_SOURCE_DIR}/include/error_types.hpp"
  "${PROJECT_SOURCE_DIR}/include/files.hpp"
  "${PROJECT_SOURCE_DIR}/include/imgui_wrapper.hpp"
  "${PROJECT_SOURCE_DIR}/include/integer_types.hpp"
  "${PROJECT_SOURCE_DIR}/include/logging.hpp"
  "${PROJECT_SOURCE_DIR}/include/module.hpp"
  "${PROJECT_SOURCE_DIR}/include/renderer.hpp"
  "${PROJECT_SOURCE_DIR}/include/tasks.hpp"
  "${PROJECT_SOURCE_DIR}/include/timers.hpp"
  "${PROJECT_SOURCE_DIR}/include/window.hpp"
)

set(
  SURGE_CORE_SOURCE_LIST
  "${PROJECT_SOURCE_DIR}/src/gl_atoms/pv_ubo.cpp"
  "${PROJECT_SOURCE_DIR}/src/gl_atoms/shaders.cpp"
  "${PROJECT_SOURCE_DIR}/src/gl_atoms/sprite.cpp"
  "${PROJECT_SOURCE_DIR}/src/gl_atoms/text.cpp"
  "${PROJECT_SOURCE_DIR}/src/gl_atoms/texture.cpp"

  "${PROJECT_SOURCE_DIR}/src/vk_bootstrap/VkBootstrap.cpp"

  "${PROJECT_SOURCE_DIR}/src/allocators.cpp"
  "${PROJECT_SOURCE_DIR}/src/cli.cpp"
  "${PROJECT_SOURCE_DIR}/src/config.cpp"
  "${PROJECT_SOURCE_DIR}/src/files.cpp"
  "${PROJECT_SOURCE_DIR}/src/imgui_wrapper.cpp"
  "${PROJECT_SOURCE_DIR}/src/module.cpp"
  "${PROJECT_SOURCE_DIR}/src/renderer_gl.cpp"
  "${PROJECT_SOURCE_DIR}/src/renderer_vk.cpp"
  "${PROJECT_SOURCE_DIR}/src/surge_core.cpp"
  "${PROJECT_SOURCE_DIR}/src/tasks.cpp"
  "${PROJECT_SOURCE_DIR}/src/timers.cpp"
  "${PROJECT_SOURCE_DIR}/src/window.cpp"
)

# -----------------------------------------
# Executable engine target
# -----------------------------------------

add_library(SurgeCore SHARED ${SURGE_CORE_HEADER_LIST} ${SURGE_CORE_SOURCE_LIST})
target_compile_features(SurgeCore PRIVATE cxx_std_20)
set_target_properties(SurgeCore PROPERTIES OUTPUT_NAME "surge_core")
set_target_properties(SurgeCore PROPERTIES PREFIX "")
set_target_properties(SurgeCore PROPERTIES PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/include/default.hpp")
set_target_properties(SurgeCore PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS True)
set_target_properties(SurgeCore PROPERTIES ENABLE_EXPORTS True)

target_include_directories(
  SurgeCore PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# Enables __VA_OPT__ on msvc
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgeCore PUBLIC /Zc:preprocessor)
endif()

# Use UTF-8 on MSVC
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgeCore PUBLIC /utf-8)
endif()

# Disable min/max macros on msvc
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgeCore PUBLIC /D NOMINMAX)
endif()

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

if(SURGE_ENABLE_SANITIZERS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeCore PUBLIC -fsanitize=address,null,unreachable,undefined)
    target_link_options(SurgeCore PUBLIC -fsanitize=address,null,unreachable,undefined)
  else()
    message(WARNING "Sanitizers don't work on MSVC yet.")
  endif()
endif()

if(SURGE_ENABLE_OPTIMIZATIONS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeCore PUBLIC -O3)
    target_link_options(SurgeCore PUBLIC -O3)
  else()
    target_compile_options(SurgeCore PUBLIC /O2)
  endif()
endif()

if(SURGE_ENABLE_TUNING)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeCore PUBLIC -march=native -mtune=native)
    target_link_options(SurgeCore PUBLIC -march=native -mtune=native)
  else()
    message(WARNING "TODO: Unknow tuning flags for msvc")
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeCore PUBLIC -flto)
    target_link_options(SurgeCore PUBLIC -flto)
  else()
    message(WARNING "TODO: Unknow LTO flag for msvc")
  endif()
endif()

if(SURGE_ENABLE_FAST_MATH)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeCore PUBLIC -ffast-math)
    target_link_options(SurgeCore PUBLIC -ffast-math)
  else()
    target_compile_options(SurgeCore PUBLIC /fp:fast)
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
      target_compile_options(
          SurgeCore
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
          -Wno-switch-enum
      )
      target_link_options(SurgeCore PUBLIC -Og -g3 -ggdb3 -rdynamic)
  else()
    target_compile_options(SurgeCore PUBLIC /Wall /MP /MTd)
    target_link_options(SurgeCore PUBLIC /DEBUG:FULL)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgeCore PUBLIC debuginfod)
  endif()

endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgeCore PUBLIC /MP /MT)
   endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Profile")
  target_link_libraries(SurgeCore PUBLIC Tracy::TracyClient)
  
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeCore PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer)
    target_link_options(SurgeCore PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer -rdynamic)
  else()
    target_compile_options(SurgeCore PUBLIC /MP /MTd)
    target_link_options(SurgeCore PUBLIC /DEBUG:FULL)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgeCore PUBLIC debuginfod)
    target_link_libraries(SurgeCore PUBLIC unwind)
  endif()
endif()

# -----------------------------------------
# Link and build order dependencies
# -----------------------------------------

target_link_libraries(SurgeCore PUBLIC
  mimalloc
  mimalloc-static
  foonathan_memory
  gsl::gsl-lite
  tl::expected
  fmt::fmt
  Taskflow::Taskflow
  xxHash::xxhash
  OpenEXR::OpenEXR
  ryml::ryml
  Freetype::Freetype
  glfw
  glad::glad
  OpenGL::GL
  Vulkan::Vulkan
  imgui::imgui
)