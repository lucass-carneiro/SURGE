cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  SurgeComputeDemoModule
  VERSION 1.4.0
  LANGUAGES CXX)

# -----------------------------------------
# Target sources
# -----------------------------------------

set(SURGE_MODULE_COMPUTE_HEADER_LIST
    "${PROJECT_SOURCE_DIR}/include/compute.hpp")

set(SURGE_MODULE_COMPUTE_SOURCE_LIST "${PROJECT_SOURCE_DIR}/src/compute.cpp")

# -----------------------------------------
# Module target
# -----------------------------------------

add_library(SurgeComputeDemoModule MODULE ${SURGE_MODULE_COMPUTE_HEADER_LIST}
                                          ${SURGE_MODULE_COMPUTE_SOURCE_LIST})
target_compile_features(SurgeComputeDemoModule PRIVATE cxx_std_20)
set_target_properties(SurgeComputeDemoModule PROPERTIES OUTPUT_NAME "compute")
set_target_properties(
  SurgeComputeDemoModule PROPERTIES PREFIX "") # removes the lib prefix on Linux
set_target_properties(SurgeComputeDemoModule PROPERTIES CXX_VISIBILITY_PRESET
                                                        hidden)
target_compile_definitions(SurgeComputeDemoModule
                           PUBLIC COMPILING_SURGE_MODULE_COMPUTE=1)

target_include_directories(
  SurgeComputeDemoModule
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>)

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

# Enables __VA_OPT__ on msvc
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
  target_compile_options(SurgeComputeDemoModule PUBLIC /Zc:preprocessor)
endif()

# Use UTF-8 on MSVC
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
  target_compile_options(SurgeComputeDemoModule PUBLIC /utf-8)
endif()

# Disable min/max macros on msvc
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
  target_compile_options(SurgeComputeDemoModule PUBLIC /D NOMINMAX)
endif()

if(SURGE_ENABLE_SANITIZERS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeComputeDemoModule
                           PUBLIC -fsanitize=address,null,unreachable,undefined)
    target_link_options(SurgeComputeDemoModule PUBLIC
                        -fsanitize=address,null,unreachable,undefined)
  else()
    # Experience reveals that using sanitizes in MSVC is an exercise in pain and
    # futility target_compile_options(SurgePlayer PUBLIC /fsanitize=address)
  endif()
endif()

if(SURGE_ENABLE_OPTIMIZATIONS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeComputeDemoModule PUBLIC -O2)
    target_link_options(SurgeComputeDemoModule PUBLIC -O2)
  else()
    target_compile_options(SurgeComputeDemoModule PUBLIC /O2)
  endif()
endif()

if(SURGE_ENABLE_TUNING)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeComputeDemoModule PUBLIC -march=native
                                                         -mtune=native)
    target_link_options(SurgeComputeDemoModule PUBLIC -march=native
                        -mtune=native)
  else()
    message(WARNING "TODO: Unknow tuning flags for msvc")
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeComputeDemoModule PUBLIC -flto)
    target_link_options(SurgeComputeDemoModule PUBLIC -flto)
  else()
    message(WARNING "TODO: Unknow LTO flag for msvc")
  endif()
endif()

if(SURGE_ENABLE_FAST_MATH)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeComputeDemoModule PUBLIC -ffast-math)
    target_link_options(SurgeComputeDemoModule PUBLIC -ffast-math)
  else()
    target_compile_options(SurgeComputeDemoModule PUBLIC /fp:fast)
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(
      SurgeComputeDemoModule
      PUBLIC -Og
             -g3
             -ggdb3
             -fno-omit-frame-pointer
             -Wall
             -Wextra
             -Wpedantic)
    target_link_options(SurgeComputeDemoModule PUBLIC -Og -g3 -ggdb3 -rdynamic)
  else()
    target_compile_options(SurgeComputeDemoModule PUBLIC /Wall /MP /MDd)
    target_link_options(SurgeComputeDemoModule PUBLIC /DEBUG:FULL)
    target_compile_options(SurgeComputeDemoModule PRIVATE /wd4820)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgeComputeDemoModule PUBLIC debuginfod)
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgeComputeDemoModule PUBLIC /MP /MD)
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Profile" OR CMAKE_BUILD_TYPE STREQUAL
                                          "RelWithDebInfo")
  target_link_libraries(SurgeComputeDemoModule PRIVATE Tracy::TracyClient)

  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeComputeDemoModule
                           PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer)
    target_link_options(SurgeComputeDemoModule PUBLIC -g3 -ggdb3
                        -fno-omit-frame-pointer -rdynamic)
  else()
    target_compile_options(SurgeComputeDemoModule PUBLIC /MP /MD)
    target_link_options(SurgeComputeDemoModule PUBLIC /DEBUG:FULL)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgeComputeDemoModule PUBLIC debuginfod)
    target_link_libraries(SurgeComputeDemoModule PUBLIC unwind)
  endif()
endif()

# -----------------------------------------
# Link and build order dependencies
# -----------------------------------------
target_link_libraries(SurgeComputeDemoModule PRIVATE SurgeCore)

# -----------------------------------------
# Shader compilation
# -----------------------------------------

set(SURGE_MODULE_COMPUTE_SHADER_LIST "${PROJECT_SOURCE_DIR}/shaders/gradient.comp")

foreach(GLSL ${SURGE_MODULE_COMPUTE_SHADER_LIST})
  message(STATUS "Building shader ${GLSL}")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  get_filename_component(FILE_DIR ${GLSL} DIRECTORY)
  set(SPIRV "${FILE_DIR}/${FILE_NAME}.spv")
  message(STATUS "Output binary of ${GLSL}: ${SPIRV}")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
    ARGS -V  ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL}
  )
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(SurgeComputeDemoModuleShaders ALL DEPENDS ${SPIRV_BINARY_FILES})
