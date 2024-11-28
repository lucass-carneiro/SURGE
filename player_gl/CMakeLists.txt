cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  SurgePlayerGL
  VERSION 1.2.0
  LANGUAGES CXX
)

# -----------------------------------------
#  Target sources
# -----------------------------------------

set(
  SURGE_PLAYER_GL_SOURCE_LIST
  "${PROJECT_SOURCE_DIR}/src/main.cpp"
)

# -----------------------------------------
# Executable engine target
# -----------------------------------------

add_executable(SurgePlayerGL ${SURGE_PLAYER_GL_SOURCE_LIST})
target_compile_features(SurgePlayerGL PRIVATE cxx_std_20)
set_target_properties(SurgePlayerGL PROPERTIES OUTPUT_NAME "surge_gl")

target_include_directories(SurgePlayerGL PRIVATE 
  $<TARGET_PROPERTY:SurgeCore,INTERFACE_INCLUDE_DIRECTORIES>
)

# Enables __VA_OPT__ on msvc
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgePlayerGL PUBLIC /Zc:preprocessor)
endif()

# Use UTF-8 on MSVC
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgePlayerGL PUBLIC /utf-8)
endif()

# Disable min/max macros on msvc
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgePlayerGL PUBLIC /D NOMINMAX)
endif()

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

if(SURGE_ENABLE_SANITIZERS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayerGL PUBLIC -fsanitize=address,null,unreachable,undefined)
    target_link_options(SurgePlayerGL PUBLIC -fsanitize=address,null,unreachable,undefined)
  else()
    message(WARNING "Sanitizers don't work on MSVC yet.")
  endif()
endif()

if(SURGE_ENABLE_OPTIMIZATIONS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayerGL PUBLIC -O3)
    target_link_options(SurgePlayerGL PUBLIC -O3)
  else()
    target_compile_options(SurgePlayerGL PUBLIC /O2)
  endif()
endif()

if(SURGE_ENABLE_TUNING)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayerGL PUBLIC -march=native -mtune=native)
    target_link_options(SurgePlayerGL PUBLIC -march=native -mtune=native)
  else()
    message(WARNING "TODO: Unknow tuning flags for msvc")
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayerGL PUBLIC -flto)
    target_link_options(SurgePlayerGL PUBLIC -flto)
  else()
    message(WARNING "TODO: Unknow LTO flag for msvc")
  endif()
endif()

if(SURGE_ENABLE_FAST_MATH)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayerGL PUBLIC -ffast-math)
    target_link_options(SurgePlayerGL PUBLIC -ffast-math)
  else()
    target_compile_options(SurgePlayerGL PUBLIC /fp:fast)
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
      target_compile_options(
          SurgePlayerGL
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
      target_link_options(SurgePlayerGL PUBLIC -Og -g3 -ggdb3 -rdynamic)
  else()
    target_compile_options(SurgePlayerGL PUBLIC /Wall /MP /MDd)
    target_link_options(SurgePlayerGL PUBLIC /DEBUG:FULL)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgePlayerGL PUBLIC debuginfod)
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgePlayerGL PUBLIC /MP /MD)
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Profile" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")  
  target_link_libraries(SurgePlayerGL PRIVATE Tracy::TracyClient)
  
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayerGL PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer)
    target_link_options(SurgePlayerGL PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer -rdynamic)
  else()
    target_compile_options(SurgePlayerGL PUBLIC /MP /MD)
    target_link_options(SurgePlayerGL PUBLIC /DEBUG:FULL)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgePlayerGL PUBLIC debuginfod)
    target_link_libraries(SurgePlayerGL PUBLIC unwind)
  endif()
endif()

# -----------------------------------------
# Link and build order dependencies
# -----------------------------------------

target_link_libraries(SurgePlayerGL PRIVATE SurgeCore)