cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  SurgeModelViewerModule
  VERSION 1.0.0
  LANGUAGES CXX
)

# -----------------------------------------
#  Target sources
# -----------------------------------------

set(
  SURGE_MODULE_DEFAULT_HEADER_LIST
  "${PROJECT_SOURCE_DIR}/include/model_viewer.hpp"
)

set(
  SURGE_MODULE_DEFAULT_SOURCE_LIST
  "${PROJECT_SOURCE_DIR}/src/model_viewer.cpp"
)

# -----------------------------------------
# Executable engine target
# -----------------------------------------

add_library(SurgeModelViewerModule MODULE ${SURGE_MODULE_DEFAULT_HEADER_LIST} ${SURGE_MODULE_DEFAULT_SOURCE_LIST})
target_compile_features(SurgeModelViewerModule PRIVATE cxx_std_20)
set_target_properties(SurgeModelViewerModule PROPERTIES PREFIX "")
set_target_properties(SurgeModelViewerModule PROPERTIES OUTPUT_NAME "model_viewer")
set_target_properties(SurgeModelViewerModule PROPERTIES PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/include/model_viewer.hpp")
set_target_properties(SurgeModelViewerModule PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_compile_definitions(SurgeModelViewerModule PUBLIC COMPILING_SURGE_MODULE_MODEL_VIEWER)

# Enables __VA_OPT__ on msvc
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgeModelViewerModule PUBLIC /Zc:preprocessor)
endif()

target_include_directories(
  SurgeModelViewerModule PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${SurgePlayer_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

target_include_directories(SurgeModelViewerModule PRIVATE ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

if(SURGE_ENABLE_SANITIZERS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModelViewerModule PUBLIC -fsanitize=address,null,unreachable,undefined)
    target_link_options(SurgeModelViewerModule PUBLIC -fsanitize=address,null,unreachable,undefined)
  else()
    message(WARNING "Sanitizers don't work on MSVC yet.")
  endif()
endif()

if(SURGE_ENABLE_OPTIMIZATIONS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModelViewerModule PUBLIC -O2)
    target_link_options(SurgeModelViewerModule PUBLIC -O2)
  else()
    target_compile_options(SurgeModelViewerModule PUBLIC /O2)
  endif()
endif()

if(SURGE_ENABLE_TUNING)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModelViewerModule PUBLIC -march=native -mtune=native)
    target_link_options(SurgeModelViewerModule PUBLIC -march=native -mtune=native)
  else()
    message(WARNING "TODO: Unknow tuning flags for msvc")
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModelViewerModule PUBLIC -flto)
    target_link_options(SurgeModelViewerModule PUBLIC -flto)
  else()
    message(WARNING "TODO: Unknow LTO flag for msvc")    
  endif()
endif()

if(SURGE_ENABLE_FAST_MATH)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModelViewerModule PUBLIC -ffast-math)
    target_link_options(SurgeModelViewerModule PUBLIC -ffast-math)
  else()
    target_compile_options(SurgeModelViewerModule PUBLIC /fp:fast)
  endif()
endif()

# Dependencies and main program build type must match. If building dependencies static-debug, then /MTd must be used.
# See the link bellow for further information
# https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2012/2kzt1wy3(v=vs.110)?redirectedfrom=MSDN

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Generating a Debug build system")

  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModelViewerModule PUBLIC -Og -g3 -ggdb3 -fno-omit-frame-pointer -Wall -Wextra -Werror -pedantic -pedantic-errors)
    target_link_options(SurgeModelViewerModule PUBLIC -Og -g3 -ggdb3 -fno-omit-frame-pointer -rdynamic -Wall -Wextra -Werror -pedantic -pedantic-errors)
  else()
    target_compile_options(SurgeModelViewerModule PUBLIC /DEBUG:FULL /MP /MDd)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgeModelViewerModule PUBLIC debuginfod)
  endif()

endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgeModelViewerModule PUBLIC /MP /MD)
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Profile")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModelViewerModule PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer)
    target_link_options(SurgeModelViewerModule PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer -rdynamic)
  else()
    target_compile_options(SurgeModelViewerModule PUBLIC /DEBUG:FULL /MP /MD)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgeModelViewerModule PUBLIC debuginfod)
  endif()
endif()

# -----------------------------------------
# Link and build order dependencies
# -----------------------------------------
target_link_libraries(SurgeModelViewerModule PRIVATE SurgePlayer)