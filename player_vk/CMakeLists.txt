cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  SurgePlayerVK
  VERSION 1.3.0
  LANGUAGES CXX
)

# -----------------------------------------
#  Target sources
# -----------------------------------------

set(
  SURGE_PLAYER_VK_SOURCE_LIST
  "${PROJECT_SOURCE_DIR}/src/main.cpp"
)

# -----------------------------------------
# Executable engine target
# -----------------------------------------

add_executable(SurgePlayerVK ${SURGE_PLAYER_VK_SOURCE_LIST})
target_compile_features(SurgePlayerVK PRIVATE cxx_std_20)
set_target_properties(SurgePlayerVK PROPERTIES OUTPUT_NAME "surge_vk")

#target_include_directories(SurgePlayerVK PRIVATE 
#  $<TARGET_PROPERTY:SurgeCore,INTERFACE_INCLUDE_DIRECTORIES>
#)

# Enables __VA_OPT__ on msvc
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgePlayerVK PUBLIC /Zc:preprocessor)
endif()

# Use UTF-8 on MSVC
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgePlayerVK PUBLIC /utf-8)
endif()

# Disable min/max macros on msvc
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgePlayerVK PUBLIC /D NOMINMAX)
endif()

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

if(SURGE_ENABLE_SANITIZERS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayerVK PUBLIC -fsanitize=address,null,unreachable,undefined)
    target_link_options(SurgePlayerVK PUBLIC -fsanitize=address,null,unreachable,undefined)
  else()
    target_compile_options(SurgePlayerVK PUBLIC /fsanitize=address)
  endif()
endif()

if(SURGE_ENABLE_OPTIMIZATIONS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayerVK PUBLIC -O3)
    target_link_options(SurgePlayerVK PUBLIC -O3)
  else()
    target_compile_options(SurgePlayerVK PUBLIC /O2)
  endif()
endif()

if(SURGE_ENABLE_TUNING)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayerVK PUBLIC -march=native -mtune=native)
    target_link_options(SurgePlayerVK PUBLIC -march=native -mtune=native)
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayerVK PUBLIC -flto)
    target_link_options(SurgePlayerVK PUBLIC -flto)
  else()
    target_compile_options(SurgePlayerVK PUBLIC /GL)
    target_link_options(SurgePlayerVK PUBLIC /LTCG)
  endif()
endif()

if(SURGE_ENABLE_FAST_MATH)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayerVK PUBLIC -ffast-math)
    target_link_options(SurgePlayerVK PUBLIC -ffast-math)
  else()
    target_compile_options(SurgePlayerVK PUBLIC /fp:fast)
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
      target_compile_options(
          SurgePlayerVK
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
      target_link_options(SurgePlayerVK PUBLIC -Og -g3 -ggdb3 -rdynamic)
  else()
    target_compile_options(SurgePlayerVK PUBLIC /Wall /MP /MDd)
    target_link_options(SurgePlayerVK PUBLIC /DEBUG:FULL)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgePlayerVK PUBLIC debuginfod)
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgePlayerVK PUBLIC /MP /MD)
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Profile" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")    
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgePlayerVK PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer)
    target_link_options(SurgePlayerVK PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer -rdynamic)
  else()
    target_compile_options(SurgePlayerVK PUBLIC /MP /MD)
    target_link_options(SurgePlayerVK PUBLIC /DEBUG:FULL)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgePlayerVK PUBLIC debuginfod)
    target_link_libraries(SurgePlayerVK PUBLIC unwind)
  endif()
endif()

# -----------------------------------------
# Link and build order dependencies
# -----------------------------------------

target_link_libraries(SurgePlayerVK PRIVATE SurgeCore)

if(SURGE_ENABLE_TRACY)
  target_link_libraries(SurgePlayerVK PUBLIC Tracy::TracyClient)
endif()  