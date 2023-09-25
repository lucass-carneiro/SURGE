cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  SurgeModule2048
  VERSION 1.0.0
  LANGUAGES CXX
)

# -----------------------------------------
#  Target sources
# -----------------------------------------

set(
  SURGE_MODULE_2048_HEADER_LIST
  "${PROJECT_SOURCE_DIR}/include/2048.hpp"
  "${PROJECT_SOURCE_DIR}/include/board.hpp"
  "${PROJECT_SOURCE_DIR}/include/debug_window.hpp"
  "${PROJECT_SOURCE_DIR}/include/globals.hpp"
  "${PROJECT_SOURCE_DIR}/include/pieces.hpp"
)

set(
  SURGE_MODULE_2048_SOURCE_LIST
  "${PROJECT_SOURCE_DIR}/src/2048.cpp"
  "${PROJECT_SOURCE_DIR}/src/board.cpp"
  "${PROJECT_SOURCE_DIR}/src/debug_window.cpp"
  "${PROJECT_SOURCE_DIR}/src/globals.cpp"
  "${PROJECT_SOURCE_DIR}/src/load_unload.cpp"
  "${PROJECT_SOURCE_DIR}/src/pieces.cpp"
)

# -----------------------------------------
# Executable engine target
# -----------------------------------------

add_library(SurgeModule2048 MODULE ${SURGE_MODULE_2048_HEADER_LIST} ${SURGE_MODULE_2048_SOURCE_LIST})
target_compile_features(SurgeModule2048 PRIVATE cxx_std_20)
set_target_properties(SurgeModule2048 PROPERTIES PREFIX "")
set_target_properties(SurgeModule2048 PROPERTIES OUTPUT_NAME "2048")
set_target_properties(SurgeModule2048 PROPERTIES PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/include/2048.hpp")
set_target_properties(SurgeModule2048 PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_compile_definitions(SurgeModule2048 PUBLIC COMPILING_SURGE_MODULE_2048)

# Enables __VA_OPT__ on msvc
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgeModule2048 PUBLIC /Zc:preprocessor)
endif()

target_include_directories(
  SurgeModule2048 PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${SurgePlayer_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

target_include_directories(SurgeModule2048 PRIVATE ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

if(SURGE_ENABLE_SANITIZERS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModule2048 PUBLIC -fsanitize=address,null,unreachable,undefined)
    target_link_options(SurgeModule2048 PUBLIC -fsanitize=address,null,unreachable,undefined)
  else()
    message(WARNING "Sanitizers don't work on MSVC yet.")# because of some "annotations" bullshit
  endif()
endif()

if(SURGE_ENABLE_OPTIMIZATIONS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModule2048 PUBLIC -O2)
    target_link_options(SurgeModule2048 PUBLIC -O2)
  else()
    target_compile_options(SurgeModule2048 PUBLIC /O2)
  endif()
endif()

if(SURGE_ENABLE_TUNING)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModule2048 PUBLIC -march=native -mtune=native)
    target_link_options(SurgeModule2048 PUBLIC -march=native -mtune=native)
  else()
    message(WARNING "TODO: Unknow tuning flags for msvc")
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModule2048 PUBLIC -flto)
    target_link_options(SurgeModule2048 PUBLIC -flto)
  else()
    message(WARNING "TODO: Unknow LTO flag for msvc")    
  endif()
endif()

if(SURGE_ENABLE_FAST_MATH)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModule2048 PUBLIC -ffast-math)
    target_link_options(SurgeModule2048 PUBLIC -ffast-math)
  else()
    target_compile_options(SurgePlayer PUBLIC /fp:fast)
  endif()
endif()

# Dependencies and main program build type must match. If building dependencies static-debug, then /MTd must be used.
# See the link bellow for further information
# https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2012/2kzt1wy3(v=vs.110)?redirectedfrom=MSDN

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Generating a Debug build system")

  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModule2048 PUBLIC -O0 -g3 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer)
    target_link_options(SurgeModule2048 PUBLIC -O0 -g3 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer)
  else()
    target_compile_options(SurgeModule2048 PUBLIC /MP /MDd)
  endif()

endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "Generating a Release build system")

  if(SURGE_ENABLE_TRACY)
    find_package(Tracy CONFIG REQUIRED)
    target_link_libraries(SurgeModule2048 PRIVATE Tracy::TracyClient)
  endif()

  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModule2048 PUBLIC -g3)
    target_link_options(SurgeModule2048 PUBLIC -g3)
  else()
    target_compile_options(SurgeModule2048 PUBLIC /MP /MD)
  endif()

endif()

# -----------------------------------------
# Link and build order dependencies
# -----------------------------------------
target_link_libraries(SurgeModule2048 PRIVATE SurgePlayer)