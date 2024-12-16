cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  SurgeCore
  VERSION 1.3.0
  LANGUAGES CXX
)

# -----------------------------------------
# Option file
# -----------------------------------------

configure_file(
  "${PROJECT_SOURCE_DIR}/include/sc_options_in.txt"
  "${PROJECT_SOURCE_DIR}/include/sc_options.hpp"
)

# -----------------------------------------
#  Target sources
# -----------------------------------------

set(
  SURGE_CORE_HEADER_LIST
  "${PROJECT_SOURCE_DIR}/include/sc_opengl/sc_opengl.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_opengl/atoms/gba.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_opengl/atoms/imgui.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_opengl/atoms/pv_ubo.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_opengl/atoms/shaders.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_opengl/atoms/sprite_database.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_opengl/atoms/text.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_opengl/atoms/texture.hpp"

  "${PROJECT_SOURCE_DIR}/include/sc_vulkan/sc_vulkan.hpp"
  "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_command.hpp"
  "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_debug.hpp"
  "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_descriptor.hpp"
  "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_images.hpp"
  "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_init.hpp"
  "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_malloc.hpp"
  "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_sync.hpp"

  "${PROJECT_SOURCE_DIR}/include/sc_allocators.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_cli.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_config.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_container_types.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_error_types.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_files.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_glfw_includes.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_glm_includes.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_imgui.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_integer_types.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_logging.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_module.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_tasks.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_timers.hpp"
  "${PROJECT_SOURCE_DIR}/include/sc_window.hpp"
)

set(
  SURGE_CORE_SOURCE_LIST
  "${PROJECT_SOURCE_DIR}/src/sc_opengl/sc_opengl.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_opengl/atoms/imgui.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_opengl/atoms/pv_ubo.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_opengl/atoms/shaders.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_opengl/atoms/sprite_database.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_opengl/atoms/text.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_opengl/atoms/texture.cpp"

  "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_clear.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_command.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_debug.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_descriptor.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_images.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_init.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_malloc.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_swpc.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_sync.cpp"

  "${PROJECT_SOURCE_DIR}/src/sc_allocators.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_cli.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_config.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_files.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_imgui.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_module.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_tasks.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_timers.cpp"
  "${PROJECT_SOURCE_DIR}/src/sc_window.cpp"
)

# -----------------------------------------
# Executable engine target
# -----------------------------------------

add_library(SurgeCore SHARED ${SURGE_CORE_HEADER_LIST} ${SURGE_CORE_SOURCE_LIST})
target_compile_features(SurgeCore PRIVATE cxx_std_20)
set_target_properties(SurgeCore PROPERTIES OUTPUT_NAME "surge_core")
set_target_properties(SurgeCore PROPERTIES PREFIX "")
set_target_properties(SurgeCore PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS True)
set_target_properties(SurgeCore PROPERTIES ENABLE_EXPORTS True)

target_include_directories(SurgeCore PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
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
# Shader compilation target
# -----------------------------------------

set(
  SURGE_CORE_SHADER_LIST
  "${PROJECT_SOURCE_DIR}/shaders/vk/compute_demo.comp"
)

foreach(GLSL ${SURGE_CORE_SHADER_LIST})
  message(STATUS "Building shader ${GLSL}")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  get_filename_component(FILE_DIR ${GLSL} DIRECTORY)
  set(SPIRV "${FILE_DIR}/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL}
  )
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(Shaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(${PROJECT_NAME} Shaders)

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
    target_compile_options(SurgeCore PUBLIC /Wall /MP /MDd)
    target_link_options(SurgeCore PUBLIC /DEBUG:FULL)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgeCore PUBLIC debuginfod)
  endif()

endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgeCore PUBLIC /MP /MD)
   endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Profile" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  target_link_libraries(SurgeCore PRIVATE Tracy::TracyClient)
  
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeCore PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer)
    target_link_options(SurgeCore PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer -rdynamic)
  else()
    target_compile_options(SurgeCore PUBLIC /MP /MD)
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
  # mimalloc needs to be the first library linked on Windows
  mimalloc-static
  mimalloc
  fmt::fmt
  Freetype::Freetype
  glad::glad
  glfw
  gsl::gsl-lite
  imgui::imgui
  OpenEXR::OpenEXR
  OpenGL::GL
  ryml::ryml
  Taskflow::Taskflow
  tl::expected
  Vulkan::Headers
  Vulkan::Vulkan
  GPUOpen::VulkanMemoryAllocator
  xxHash::xxhash
)