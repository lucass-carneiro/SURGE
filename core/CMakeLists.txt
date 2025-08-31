cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  SurgeCore
  VERSION 1.4.0
  LANGUAGES CXX)

# -----------------------------------------
# Option file
# -----------------------------------------

configure_file("${PROJECT_SOURCE_DIR}/include/sc_options_in.txt"
               "${PROJECT_SOURCE_DIR}/include/sc_options.hpp")

# -----------------------------------------
# Target sources
# -----------------------------------------

set(SURGE_CORE_HEADER_LIST
    "${PROJECT_SOURCE_DIR}/include/sc_vulkan/sc_vulkan.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_vulkan/sc_vulkan_command.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_vulkan/sc_vulkan_debug.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_vulkan/sc_vulkan_descriptor.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_vulkan/sc_vulkan_images.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_vulkan/sc_vulkan_init.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_vulkan/sc_vulkan_malloc.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_vulkan/sc_vulkan_pipelines.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_vulkan/sc_vulkan_sync.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_vulkan/sc_vulkan_types.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_vulkan/atoms/sprite_database.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_allocators.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_cli.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_config.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_container_types.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_error_types.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_files.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_glm.hpp"
    # "${PROJECT_SOURCE_DIR}/include/sc_imgui.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_integer_types.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_logging.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_module.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_random.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_module_context.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_timers.hpp"
    "${PROJECT_SOURCE_DIR}/include/sc_window.hpp")

set(SURGE_CORE_SOURCE_LIST
    "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_buffer.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_clear.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_command.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_create_destroy.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_debug.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_descriptor.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_images.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_init.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_malloc.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_pipelines.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_swpc.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_vulkan/sc_vulkan_sync.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_vulkan/atoms/sprite_database.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_allocators.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_cli.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_config.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_files.cpp"
    # "${PROJECT_SOURCE_DIR}/src/sc_imgui.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_module.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_random.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_timers.cpp"
    "${PROJECT_SOURCE_DIR}/src/sc_window.cpp")

# -----------------------------------------
# Executable engine target
# -----------------------------------------

add_library(SurgeCore SHARED ${SURGE_CORE_HEADER_LIST}
                             ${SURGE_CORE_SOURCE_LIST})
target_compile_features(SurgeCore PRIVATE cxx_std_20)
set_target_properties(SurgeCore PROPERTIES OUTPUT_NAME "surge_core")
set_target_properties(SurgeCore PROPERTIES PREFIX "")
set_target_properties(SurgeCore PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS True)
set_target_properties(SurgeCore PROPERTIES ENABLE_EXPORTS True)

target_include_directories(
  SurgeCore
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>)

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

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

# This warning alerts us on the unsafeness of casting a void* to a function and
# calling it. It is impossible to have dynamically loaded modules without these
# casts, and we acknowledge that this is unsafe. Given that, we disable that
# warning here
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
  target_compile_options(SurgeCore PRIVATE /wd4191)
endif()

if(SURGE_ENABLE_SANITIZERS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeCore
                           PUBLIC -fsanitize=address,null,unreachable,undefined)
    target_link_options(SurgeCore PUBLIC
                        -fsanitize=address,null,unreachable,undefined)
  else()
    # Experience reveals that using sanitizes in MSVC is an exercise in pain and
    # futility target_compile_options(SurgeCore PUBLIC /fsanitize=address)
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
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeCore PUBLIC -flto)
    target_link_options(SurgeCore PUBLIC -flto)
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
      PUBLIC -Og
             -g3
             -ggdb3
             -fno-omit-frame-pointer
             -Wall
             -Wextra
             -Wpedantic)
    target_link_options(SurgeCore PUBLIC -Og -g3 -ggdb3 -rdynamic)
  else()
    target_compile_options(SurgeCore PRIVATE /Wall /MP /MDd)
    target_link_options(SurgeCore PRIVATE /DEBUG:FULL)
    target_compile_options(SurgeCore PRIVATE /wd4820)
    target_compile_options(SurgeCore PRIVATE /wd4866)
    target_compile_options(SurgeCore PRIVATE /wd5045)
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

if(CMAKE_BUILD_TYPE STREQUAL "Profile" OR CMAKE_BUILD_TYPE STREQUAL
                                          "RelWithDebInfo")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeCore PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer)
    target_link_options(SurgeCore PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer
                        -rdynamic)
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

target_link_libraries(
  SurgeCore
  PUBLIC # mimalloc needs to be the first library linked on Windows
         mimalloc-static
         mimalloc
         fmt::fmt
         Freetype::Freetype
         glfw
         imgui::imgui
         ryml::ryml
         tl::expected
         Vulkan::Vulkan
         Vulkan::Headers
         GPUOpen::VulkanMemoryAllocator
         xxHash::xxhash)

if(SURGE_ENABLE_TRACY)
  target_link_libraries(SurgeCore PUBLIC Tracy::TracyClient)
endif()

# -----------------------------------------
# Shader compilation
# -----------------------------------------

set(SURGE_CORE_SHADER_LIST "${PROJECT_SOURCE_DIR}/shaders/sprite_database.vert"
                           "${PROJECT_SOURCE_DIR}/shaders/sprite_database.frag")

foreach(GLSL ${SURGE_CORE_SHADER_LIST})
  message(STATUS "Building shader ${GLSL}")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  get_filename_component(FILE_DIR ${GLSL} DIRECTORY)
  set(SPIRV "${FILE_DIR}/${FILE_NAME}.spv")
  message(STATUS "Output binary of ${GLSL}: ${SPIRV}")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND CORE_SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(SurgeCoreShaders ALL DEPENDS ${CORE_SPIRV_BINARY_FILES})
