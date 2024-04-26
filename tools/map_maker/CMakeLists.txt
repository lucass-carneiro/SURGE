cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  SurgeMapMaker
  VERSION 1.0.0
  LANGUAGES CXX
)

# -----------------------------------------
#  Target sources
# -----------------------------------------

set(
  SURGE_TOOL_MAP_MAKER_INCLUDE_LIST
  "${PROJECT_SOURCE_DIR}/src/depth_data_file.hpp"
)

set(
  SURGE_TOOL_MAP_MAKER_SOURCE_LIST
  "${PROJECT_SOURCE_DIR}/src/main.cpp"
)

# -----------------------------------------
# Module Target
# -----------------------------------------

add_executable(SurgeMapMaker ${SURGE_TOOL_MAP_MAKER_INCLUDE_LIST} ${SURGE_TOOL_MAP_MAKER_SOURCE_LIST})
target_compile_features(SurgeMapMaker PRIVATE cxx_std_20)
set_target_properties(SurgeMapMaker PROPERTIES OUTPUT_NAME "map_maker")

# Enables __VA_OPT__ on msvc
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgeMapMaker PUBLIC /Zc:preprocessor)
endif()

# Use UTF-8 on MSVC
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgeMapMaker PUBLIC /utf-8)
endif()

target_include_directories(SurgeMapMaker PRIVATE ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

if(SURGE_ENABLE_SANITIZERS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeMapMaker PUBLIC -fsanitize=address,null,unreachable,undefined)
    target_link_options(SurgeMapMaker PUBLIC -fsanitize=address,null,unreachable,undefined)
  else()
    message(WARNING "Sanitizers don't work on MSVC yet.")
  endif()
endif()

if(SURGE_ENABLE_OPTIMIZATIONS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeMapMaker PUBLIC -O3)
    target_link_options(SurgeMapMaker PUBLIC -O3)
  else()
    target_compile_options(SurgeMapMaker PUBLIC /O2)
  endif()
endif()

if(SURGE_ENABLE_TUNING)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeMapMaker PUBLIC -march=native -mtune=native)
    target_link_options(SurgeMapMaker PUBLIC -march=native -mtune=native)
  else()
    message(WARNING "TODO: Unknow tuning flags for msvc")
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeMapMaker PUBLIC -flto)
    target_link_options(SurgeMapMaker PUBLIC -flto)
  else()
    message(WARNING "TODO: Unknow LTO flag for msvc")
  endif()
endif()

if(SURGE_ENABLE_FAST_MATH)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeMapMaker PUBLIC -ffast-math)
    target_link_options(SurgeMapMaker PUBLIC -ffast-math)
  else()
    target_compile_options(SurgeMapMaker PUBLIC /fp:fast)
  endif()
endif()

# Dependencies and main program build type must match.
# If building dependencies static-debug, then /MTd must be used.
# See the link bellow for further information
# https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2012/2kzt1wy3(v=vs.110)?redirectedfrom=MSDN

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
      target_compile_options(
          SurgeMapMaker
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
      target_link_options(SurgeMapMaker PUBLIC -Og -g3 -ggdb3)
  else()
    target_compile_options(SurgeMapMaker PUBLIC /Wall /MP /MDd)
    target_link_options(SurgeMapMaker PUBLIC /DEBUG:FULL)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgeMapMaker PUBLIC debuginfod)
  endif()

endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    message(WARNING ${VCPKG_LIBRARY_LINKAGE})
    if(VCPKG_TARGET_TRIPLET MATCHES "x64-windows-static") #TODO: This is not general and static releases do not work
      target_compile_options(SurgeMapMaker PUBLIC /MP /MT)
    else()
      target_compile_options(SurgeMapMaker PUBLIC /MP /MD)
    endif()
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Profile")
  target_link_libraries(SurgeMapMaker PUBLIC Tracy::TracyClient)
  
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeMapMaker PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer)
    target_link_options(SurgeMapMaker PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer)
  else()
    target_compile_options(SurgeMapMaker PUBLIC /MP /MD)
    target_link_options(SurgeMapMaker PUBLIC /DEBUG:FULL)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgeMapMaker PUBLIC debuginfod)
    target_link_libraries(SurgeMapMaker PUBLIC unwind)
  endif()
endif()

# -----------------------------------------
# Link and build order dependencies
# -----------------------------------------
target_link_libraries(SurgeMapMaker PRIVATE
  OpenEXR::OpenEXR
  docopt
  fmt::fmt
)
