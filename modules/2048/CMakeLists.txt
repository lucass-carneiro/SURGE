cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  Surge2048Module
  VERSION 1.0.0
  LANGUAGES CXX
)

# -----------------------------------------
#  Target sources
# -----------------------------------------

set(
  SURGE_MODULE_2048_HEADER_LIST
  "${PROJECT_SOURCE_DIR}/include/2048.hpp"
  "${PROJECT_SOURCE_DIR}/include/pieces.hpp"
  "${PROJECT_SOURCE_DIR}/include/score.hpp"
)

set(
  SURGE_MODULE_2048_SOURCE_LIST
  "${PROJECT_SOURCE_DIR}/src/2048.cpp"
  "${PROJECT_SOURCE_DIR}/src/draw.cpp"
  "${PROJECT_SOURCE_DIR}/src/globals.cpp"
  "${PROJECT_SOURCE_DIR}/src/pieces_down_motion.cpp"
  "${PROJECT_SOURCE_DIR}/src/pieces_left_motion.cpp"
  "${PROJECT_SOURCE_DIR}/src/pieces_right_motion.cpp"
  "${PROJECT_SOURCE_DIR}/src/pieces_up_motion.cpp"
  "${PROJECT_SOURCE_DIR}/src/pieces.cpp"
  "${PROJECT_SOURCE_DIR}/src/score.cpp"
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  list(APPEND SURGE_MODULE_2048_HEADER_LIST "${PROJECT_SOURCE_DIR}/include/debug_window.hpp")
  list(APPEND SURGE_MODULE_2048_SOURCE_LIST "${PROJECT_SOURCE_DIR}/src/debug_window.cpp")
endif()

# -----------------------------------------
# Executable engine target
# -----------------------------------------

add_library(Surge2048Module MODULE ${SURGE_MODULE_2048_HEADER_LIST} ${SURGE_MODULE_2048_SOURCE_LIST})
target_compile_features(Surge2048Module PRIVATE cxx_std_20)
set_target_properties(Surge2048Module PROPERTIES PREFIX "")
set_target_properties(Surge2048Module PROPERTIES OUTPUT_NAME "2048")
set_target_properties(Surge2048Module PROPERTIES PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/include/2048.hpp")
set_target_properties(Surge2048Module PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_compile_definitions(Surge2048Module PUBLIC COMPILING_SURGE_MODULE_2048)

# Enables __VA_OPT__ on msvc
if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(Surge2048Module PUBLIC /Zc:preprocessor)
endif()

target_include_directories(
  Surge2048Module PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${SurgePlayer_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

target_include_directories(Surge2048Module PRIVATE ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

if(SURGE_ENABLE_SANITIZERS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge2048Module PUBLIC -fsanitize=address,null,unreachable,undefined)
    target_link_options(Surge2048Module PUBLIC -fsanitize=address,null,unreachable,undefined)
  else()
    message(WARNING "Sanitizers don't work on MSVC yet.")
  endif()
endif()

if(SURGE_ENABLE_OPTIMIZATIONS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge2048Module PUBLIC -O3)
    target_link_options(Surge2048Module PUBLIC -O3)
  else()
    target_compile_options(Surge2048Module PUBLIC /O2)
  endif()
endif()

if(SURGE_ENABLE_TUNING)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge2048Module PUBLIC -march=native -mtune=native)
    target_link_options(Surge2048Module PUBLIC -march=native -mtune=native)
  else()
    message(WARNING "TODO: Unknow tuning flags for msvc")
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge2048Module PUBLIC -flto)
    target_link_options(Surge2048Module PUBLIC -flto)
  else()
    message(WARNING "TODO: Unknow LTO flag for msvc")    
  endif()
endif()

if(SURGE_ENABLE_FAST_MATH)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge2048Module PUBLIC -ffast-math)
    target_link_options(Surge2048Module PUBLIC -ffast-math)
  else()
    target_compile_options(Surge2048Module PUBLIC /fp:fast)
  endif()
endif()

# Dependencies and main program build type must match. If building dependencies static-debug, then /MTd must be used.
# See the link bellow for further information
# https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2012/2kzt1wy3(v=vs.110)?redirectedfrom=MSDN

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Generating a Debug build system")

  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge2048Module PUBLIC -Og -g3 -ggdb3 -fno-omit-frame-pointer -Wall -Wextra -Werror -pedantic -pedantic-errors)
    target_link_options(Surge2048Module PUBLIC -Og -g3 -ggdb3 -fno-omit-frame-pointer -rdynamic -Wall -Wextra -Werror -pedantic -pedantic-errors)
  else()
    target_compile_options(Surge2048Module PUBLIC /Wall /MP /MDd)
    target_link_options(Surge2048Module PUBLIC /DEBUG:FULL)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(Surge2048Module PUBLIC debuginfod)
  endif()

endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "msvc")
    target_compile_options(Surge2048Module PUBLIC /MP /MD)
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Profile")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge2048Module PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer)
    target_link_options(Surge2048Module PUBLIC -g3 -ggdb3 -fno-omit-frame-pointer -rdynamic)
  else()
    target_compile_options(Surge2048Module PUBLIC /MP /MD)
    target_link_options(Surge2048Module PUBLIC /DEBUG:FULL)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(Surge2048Module PUBLIC debuginfod)
  endif()
endif()

# -----------------------------------------
# Link and build order dependencies
# -----------------------------------------
target_link_libraries(Surge2048Module PRIVATE SurgePlayer)