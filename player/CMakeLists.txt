cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  SurgePlayer
  VERSION 1.3.0
  LANGUAGES CXX
)

# -----------------------------------------
#  Target sources
# -----------------------------------------

set(
  SURGE_PLAYER_SOURCE_LIST
  "${PROJECT_SOURCE_DIR}/src/main.cpp"
)

# -----------------------------------------
# Executable engine target
# -----------------------------------------

add_executable(SurgePlayer ${SURGE_PLAYER_SOURCE_LIST})
target_compile_features(SurgePlayer PRIVATE cxx_std_20)
set_target_properties(SurgePlayer PROPERTIES OUTPUT_NAME "surge")

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

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

if(SURGE_ENABLE_SANITIZERS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayer PUBLIC -fsanitize=address,null,unreachable,undefined)
    target_link_options(SurgePlayer PUBLIC -fsanitize=address,null,unreachable,undefined)
  else()
    # Experience reveals that using sanitizes in MSVC is an exercise in pain and futility
    #target_compile_options(SurgePlayer PUBLIC /fsanitize=address)
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
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayer PUBLIC -flto)
    target_link_options(SurgePlayer PUBLIC -flto)
  else()
    target_compile_options(SurgePlayer PUBLIC /GL)
    target_link_options(SurgePlayer PUBLIC /LTCG)
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
    target_compile_options(SurgePlayer PRIVATE /Wall /MP /MDd)
    target_link_options(SurgePlayer PRIVATE /DEBUG:FULL)
    target_compile_options(SurgePlayer PRIVATE /wd4820)
    target_compile_options(SurgePlayer PRIVATE /wd4866)
    target_compile_options(SurgePlayer PRIVATE /wd5045)
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

if(CMAKE_BUILD_TYPE STREQUAL "Profile" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")    
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

target_link_libraries(SurgePlayer PRIVATE SurgeCore)

if(SURGE_ENABLE_TRACY)
  target_link_libraries(SurgePlayer PUBLIC Tracy::TracyClient)
endif()  