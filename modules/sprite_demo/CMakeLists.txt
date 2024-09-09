cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  SurgeSpriteDemoModule
  VERSION 1.0.0
  LANGUAGES CXX
)

# -----------------------------------------
#  Target sources
# -----------------------------------------

set(
  SURGE_MODULE_SPRITE_DEMO_HEADER_LIST
  "${SurgeCore_SOURCE_DIR}/include/surge_core.hpp"
  "${PROJECT_SOURCE_DIR}/include/sprite_demo.hpp"
)

set(
  SURGE_MODULE_SPRITE_DEMO_SOURCE_LIST
  "${PROJECT_SOURCE_DIR}/src/sprite_demo.cpp"
)

# -----------------------------------------
# Executable engine target
# -----------------------------------------

add_library(SurgeSpriteDemoModule MODULE ${SURGE_MODULE_SPRITE_DEMO_HEADER_LIST} ${SURGE_MODULE_SPRITE_DEMO_SOURCE_LIST})
target_compile_features(SurgeSpriteDemoModule PRIVATE cxx_std_20)
set_target_properties(SurgeSpriteDemoModule PROPERTIES PREFIX "")
set_target_properties(SurgeSpriteDemoModule PROPERTIES OUTPUT_NAME "sprite_demo")
set_target_properties(SurgeSpriteDemoModule PROPERTIES PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/include/sprite_demo.hpp")
set_target_properties(SurgeSpriteDemoModule PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_compile_definitions(SurgeSpriteDemoModule PUBLIC COMPILING_SURGE_MODULE_SPRITE_DEMO)

target_include_directories(
  SurgeSpriteDemoModule PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# Enables __VA_OPT__ on msvc
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgeSpriteDemoModule PUBLIC /Zc:preprocessor)
endif()

# Use UTF-8 on MSVC
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgeSpriteDemoModule PUBLIC /utf-8)
endif()

# Disable min/max macros on msvc
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgeSpriteDemoModule PUBLIC /D NOMINMAX)
endif()

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

if(SURGE_ENABLE_SANITIZERS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeSpriteDemoModule PUBLIC -fsanitize=address,null,unreachable,undefined)
    target_link_options(SurgeSpriteDemoModule PUBLIC -fsanitize=address,null,unreachable,undefined)
  else()
    message(WARNING "Sanitizers don't work on MSVC yet.")
  endif()
endif()

if(SURGE_ENABLE_OPTIMIZATIONS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeSpriteDemoModule PUBLIC -O2)
    target_link_options(SurgeSpriteDemoModule PUBLIC -O2)
  else()
    target_compile_options(SurgeSpriteDemoModule PUBLIC /O2)
  endif()
endif()

if(SURGE_ENABLE_TUNING)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeSpriteDemoModule PUBLIC -march=native -mtune=native)
    target_link_options(SurgeSpriteDemoModule PUBLIC -march=native -mtune=native)
  else()
    message(WARNING "TODO: Unknow tuning flags for msvc")
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeSpriteDemoModule PUBLIC -flto)
    target_link_options(SurgeSpriteDemoModule PUBLIC -flto)
  else()
    message(WARNING "TODO: Unknow LTO flag for msvc")    
  endif()
endif()

if(SURGE_ENABLE_FAST_MATH)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeSpriteDemoModule PUBLIC -ffast-math)
    target_link_options(SurgeSpriteDemoModule PUBLIC -ffast-math)
  else()
    target_compile_options(SurgeSpriteDemoModule PUBLIC /fp:fast)
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
      target_compile_options(
          SurgeSpriteDemoModule
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
      target_link_options(SurgeSpriteDemoModule PUBLIC -Og -g3 -ggdb3 -rdynamic)
  else()
    target_compile_options(SurgeSpriteDemoModule PUBLIC /Wall /MP /MDd)
    target_link_options(SurgeSpriteDemoModule PUBLIC /DEBUG:FULL)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgeSpriteDemoModule PUBLIC debuginfod)
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgeSpriteDemoModule PUBLIC /MP /MD)
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Profile" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")  
  target_link_libraries(SurgeSpriteDemoModule PRIVATE Tracy::TracyClient)
  
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeSpriteDemoModule PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer)
    target_link_options(SurgeSpriteDemoModule PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer -rdynamic)
  else()
    target_compile_options(SurgeSpriteDemoModule PUBLIC /MP /MD)
    target_link_options(SurgeSpriteDemoModule PUBLIC /DEBUG:FULL)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgeSpriteDemoModule PUBLIC debuginfod)
    target_link_libraries(SurgeSpriteDemoModule PUBLIC unwind)
  endif()
endif()

# -----------------------------------------
# Link and build order dependencies
# -----------------------------------------
target_link_libraries(SurgeSpriteDemoModule PRIVATE SurgeCore)