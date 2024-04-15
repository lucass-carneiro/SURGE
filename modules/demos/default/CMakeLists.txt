cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  SurgeDefaultModule
  VERSION 1.0.0
  LANGUAGES CXX
)

# -----------------------------------------
#  Target sources
# -----------------------------------------

set(
  SURGE_MODULE_DEFAULT_HEADER_LIST
  "${PROJECT_SOURCE_DIR}/include/debug_window.hpp"
  "${PROJECT_SOURCE_DIR}/include/default.hpp"
)

set(
  SURGE_MODULE_DEFAULT_SOURCE_LIST
  "${PROJECT_SOURCE_DIR}/src/debug_window.cpp"
  "${PROJECT_SOURCE_DIR}/src/default.cpp"
)

# -----------------------------------------
# Link Target
# -----------------------------------------

# This target creates links to the player include files in the
# module source tree. This avoids name conflicts between includes

add_custom_target(
  SurgeDefaultModule_link_player_includes
  ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${SurgePlayer_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/include/player
)

# -----------------------------------------
# Executable engine target
# -----------------------------------------

add_library(SurgeDefaultModule MODULE ${SURGE_MODULE_DEFAULT_HEADER_LIST} ${SURGE_MODULE_DEFAULT_SOURCE_LIST})
target_compile_features(SurgeDefaultModule PRIVATE cxx_std_20)
set_target_properties(SurgeDefaultModule PROPERTIES PREFIX "")
set_target_properties(SurgeDefaultModule PROPERTIES OUTPUT_NAME "default")
set_target_properties(SurgeDefaultModule PROPERTIES PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/include/default.hpp")
set_target_properties(SurgeDefaultModule PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_compile_definitions(SurgeDefaultModule PUBLIC COMPILING_SURGE_MODULE_DEFAULT)

# Enables __VA_OPT__ on msvc
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgeDefaultModule PUBLIC /Zc:preprocessor)
endif()

target_include_directories(
  SurgeDefaultModule PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

target_include_directories(SurgeDefaultModule PRIVATE ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

if(SURGE_ENABLE_SANITIZERS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeDefaultModule PUBLIC -fsanitize=address,null,unreachable,undefined)
    target_link_options(SurgeDefaultModule PUBLIC -fsanitize=address,null,unreachable,undefined)
  else()
    message(WARNING "Sanitizers don't work on MSVC yet.")
  endif()
endif()

if(SURGE_ENABLE_OPTIMIZATIONS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeDefaultModule PUBLIC -O2)
    target_link_options(SurgeDefaultModule PUBLIC -O2)
  else()
    target_compile_options(SurgeDefaultModule PUBLIC /O2)
  endif()
endif()

if(SURGE_ENABLE_TUNING)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeDefaultModule PUBLIC -march=native -mtune=native)
    target_link_options(SurgeDefaultModule PUBLIC -march=native -mtune=native)
  else()
    message(WARNING "TODO: Unknow tuning flags for msvc")
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeDefaultModule PUBLIC -flto)
    target_link_options(SurgeDefaultModule PUBLIC -flto)
  else()
    message(WARNING "TODO: Unknow LTO flag for msvc")    
  endif()
endif()

if(SURGE_ENABLE_FAST_MATH)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeDefaultModule PUBLIC -ffast-math)
    target_link_options(SurgeDefaultModule PUBLIC -ffast-math)
  else()
    target_compile_options(SurgeDefaultModule PUBLIC /fp:fast)
  endif()
endif()

# Dependencies and main program build type must match. If building dependencies static-debug, then /MTd must be used.
# See the link bellow for further information
# https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2012/2kzt1wy3(v=vs.110)?redirectedfrom=MSDN

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Generating a Debug build system")

  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeDefaultModule PUBLIC -Og -g3 -ggdb3 -fno-omit-frame-pointer -Wall -Wextra -Werror -pedantic -pedantic-errors)
    target_link_options(SurgeDefaultModule PUBLIC -Og -g3 -ggdb3 -fno-omit-frame-pointer -rdynamic -Wall -Wextra -Werror -pedantic -pedantic-errors)
  else()
    target_compile_options(SurgeDefaultModule PUBLIC /Wall /MP /MDd)
    target_link_options(SurgeDefaultModule PUBLIC /DEBUG:FULL)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgeDefaultModule PUBLIC debuginfod)
  endif()

endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    message(WARNING ${VCPKG_LIBRARY_LINKAGE})
    if(VCPKG_TARGET_TRIPLET MATCHES "x64-windows-static") #TODO: This is not general and static releases do not work
      target_compile_options(SurgeDefaultModule PUBLIC /MP /MT)
    else()
      target_compile_options(SurgeDefaultModule PUBLIC /MP /MD)
    endif()
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Profile")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeDefaultModule PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer)
    target_link_options(SurgeDefaultModule PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer -rdynamic)
  else()
    target_compile_options(SurgeDefaultModule PUBLIC /MP /MD)
    target_link_options(SurgeDefaultModule PUBLIC /DEBUG:FULL)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(SurgeDefaultModule PUBLIC debuginfod)
  endif()
endif()

# -----------------------------------------
# Link and build order dependencies
# -----------------------------------------
target_link_libraries(SurgeDefaultModule PRIVATE SurgePlayer)