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
#  Target sources
# -----------------------------------------

set(
  SURGE_PLAYER_HEADER_LIST
  "${SurgeCore_SOURCE_DIR}/include/options.hpp"
  "${SurgeCore_SOURCE_DIR}/include/surge_core.hpp"
)

set(
  SURGE_PLAYER_SOURCE_LIST
  "${PROJECT_SOURCE_DIR}/src/main.cpp"
)

# -----------------------------------------
# Executable engine target
# -----------------------------------------

add_executable(SurgePlayer ${SURGE_PLAYER_HEADER_LIST} ${SURGE_PLAYER_SOURCE_LIST})
target_compile_features(SurgePlayer PRIVATE cxx_std_20)
set_target_properties(SurgePlayer PROPERTIES OUTPUT_NAME "surge")

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
    target_compile_options(SurgePlayer PUBLIC /MP /MD)
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Profile")
  target_link_libraries(SurgePlayer PUBLIC Tracy::TracyClient)
  
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayer PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer)
    target_link_options(SurgePlayer PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer -rdynamic)
  else()
    target_compile_options(SurgePlayer PUBLIC /MP /MDd)
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

target_link_libraries(SurgePlayer PRIVATE SurgeCore)