cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  SurgeModuleLasers
  VERSION 1.0.0
  LANGUAGES CXX
)

# -----------------------------------------
#  Target sources
# -----------------------------------------

set(
  SURGE_MODULE_LASERS_HEADER_LIST
  "${PROJECT_SOURCE_DIR}/include/lasers.hpp"
)

set(
  SURGE_MODULE_LASERS_SOURCE_LIST
  "${PROJECT_SOURCE_DIR}/src/draw.cpp"
  "${PROJECT_SOURCE_DIR}/src/lasers.cpp"
  "${PROJECT_SOURCE_DIR}/src/on_load.cpp"
  "${PROJECT_SOURCE_DIR}/src/on_unload.cpp"
)

# -----------------------------------------
# Executable engine target
# -----------------------------------------

add_library(SurgeModuleLasers MODULE ${SURGE_MODULE_LASERS_HEADER_LIST} ${SURGE_MODULE_LASERS_SOURCE_LIST})
target_compile_features(SurgeModuleLasers PRIVATE cxx_std_20)
set_target_properties(SurgeModuleLasers PROPERTIES PREFIX "")
set_target_properties(SurgeModuleLasers PROPERTIES OUTPUT_NAME "lasers")
set_target_properties(SurgeModuleLasers PROPERTIES PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/include/lasers.hpp")
set_target_properties(SurgeModuleLasers PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_compile_definitions(SurgeModuleLasers PUBLIC COMPILING_SURGE_MODULE_LASERS)

# Enables __VA_OPT__ on msvc
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(SurgeModuleLasers PUBLIC /Zc:preprocessor)
endif()

target_include_directories(
  SurgeModuleLasers PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${SurgePlayer_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

target_include_directories(SurgeModuleLasers PRIVATE ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

if(SURGE_ENABLE_SANITIZERS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModuleLasers PUBLIC -fsanitize=address,null,unreachable,undefined)
    target_link_options(SurgeModuleLasers PUBLIC -fsanitize=address,null,unreachable,undefined)
  else()
    message(WARNING "Sanitizers don't work on MSVC yet.")# because of some "annotations" bullshit
  endif()
endif()

if(SURGE_ENABLE_OPTIMIZATIONS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModuleLasers PUBLIC -O2)
    target_link_options(SurgeModuleLasers PUBLIC -O2)
  else()
    target_compile_options(SurgeModuleLasers PUBLIC /O2)
  endif()
endif()

if(SURGE_ENABLE_TUNING)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModuleLasers PUBLIC -march=native -mtune=native)
    target_link_options(SurgeModuleLasers PUBLIC -march=native -mtune=native)
  else()
    message(WARNING "TODO: Unknow tuning flags for msvc")
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModuleLasers PUBLIC -flto)
    target_link_options(SurgeModuleLasers PUBLIC -flto)
  else()
    message(WARNING "TODO: Unknow LTO flag for msvc")    
  endif()
endif()

if(SURGE_ENABLE_FAST_MATH)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModuleLasers PUBLIC -ffast-math)
    target_link_options(SurgeModuleLasers PUBLIC -ffast-math)
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
    target_compile_options(SurgeModuleLasers PUBLIC -O0 -g3 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer)
    target_link_options(SurgeModuleLasers PUBLIC -O0 -g3 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer)
  else()
    target_compile_options(SurgeModuleLasers PUBLIC /MP /MDd)
  endif()

endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "Generating a Release build system")

  if(SURGE_ENABLE_TRACY)
    find_package(Tracy CONFIG REQUIRED)
    target_link_libraries(SurgeModuleLasers PRIVATE Tracy::TracyClient)
  endif()

  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeModuleLasers PUBLIC -g3)
    target_link_options(SurgeModuleLasers PUBLIC -g3)
  else()
    target_compile_options(SurgeModuleLasers PUBLIC /MP /MD)
  endif()

endif()

# -----------------------------------------
# Link and build order dependencies
# -----------------------------------------
target_link_libraries(SurgeModuleLasers PRIVATE SurgePlayer)