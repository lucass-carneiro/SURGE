cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  Surge2048
  VERSION 1.0.0
  LANGUAGES CXX
)

# -----------------------------------------
#  Target sources
# -----------------------------------------

set(
  SURGE_MODULE_DEFAULT_HEADER_LIST
  "${PROJECT_SOURCE_DIR}/include/2048_board.hpp"
  "${PROJECT_SOURCE_DIR}/include/2048_debug_window.hpp"
  "${PROJECT_SOURCE_DIR}/include/2048_globals.hpp"
  "${PROJECT_SOURCE_DIR}/include/2048_pieces.hpp"
  "${PROJECT_SOURCE_DIR}/include/2048.hpp"
)

set(
  SURGE_MODULE_DEFAULT_SOURCE_LIST
  "${PROJECT_SOURCE_DIR}/source/2048_board.cpp"
  "${PROJECT_SOURCE_DIR}/source/2048_callbacks.cpp"
  "${PROJECT_SOURCE_DIR}/source/2048_debug_window.cpp"
  "${PROJECT_SOURCE_DIR}/source/2048_draw.cpp"
  "${PROJECT_SOURCE_DIR}/source/2048_globals.cpp"
  "${PROJECT_SOURCE_DIR}/source/2048_load_unload.cpp"
  "${PROJECT_SOURCE_DIR}/source/2048_pieces.cpp"
)

# -----------------------------------------
# Executable engine target
# -----------------------------------------

add_library(Surge2048 MODULE ${SURGE_MODULE_DEFAULT_HEADER_LIST} ${SURGE_MODULE_DEFAULT_SOURCE_LIST})
target_compile_features(Surge2048 PRIVATE cxx_std_20)
set_target_properties(Surge2048 PROPERTIES PREFIX "")
set_target_properties(Surge2048 PROPERTIES OUTPUT_NAME "2048")
set_target_properties(Surge2048 PROPERTIES PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/include/2048.hpp")
set_target_properties(Surge2048 PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_compile_definitions(Surge2048 PUBLIC COMPILING_SURGE_MODULE_2048)


target_include_directories(
  Surge2048 PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${SurgePlayer_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

if(SURGE_ENABLE_SANITIZERS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge2048 PUBLIC -fsanitize=address,null,unreachable,undefined)
    target_link_options(Surge2048 PUBLIC -fsanitize=address,null,unreachable,undefined)
  else()
    # TODO
  endif()
endif()

if(SURGE_ENABLE_OPTIMIZATIONS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge2048 PUBLIC -O2)
    target_link_options(Surge2048 PUBLIC -O2)
  else()
    target_compile_options(Surge2048 PUBLIC /O2)
  endif()
endif()

if(SURGE_ENABLE_TUNING)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge2048 PUBLIC -march=native -mtune=native)
    target_link_options(Surge2048 PUBLIC -march=native -mtune=native)
  else()
    # TODO
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge2048 PUBLIC -flto)
    target_link_options(Surge2048 PUBLIC -flto)
  else()
    # TODO
  endif()
endif()

if(SURGE_ENABLE_FAST_MATH)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge2048 PUBLIC -ffast-math)
    target_link_options(Surge2048 PUBLIC -ffast-math)
  else()
    # TODO
  endif()
endif()

# Dependencies and main program build type must match. If building dependencies static-debug, then /MTd must be used.
# See the link bellow for further information
# https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2012/2kzt1wy3(v=vs.110)?redirectedfrom=MSDN

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Generating a Debug build system")

  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge2048 PUBLIC -O0 -g3 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer)
    target_link_options(Surge2048 PUBLIC -O0 -g3 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer)
  else()
    target_compile_options(Surge2048 PUBLIC /MP /MDd)
  endif()

endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "Generating a Release build system")

  if(SURGE_ENABLE_TRACY)
    find_package(Tracy CONFIG REQUIRED)
    target_link_libraries(Surge2048 PRIVATE Tracy::TracyClient)
  endif()

  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge2048 PUBLIC -g3)
    target_link_options(Surge2048 PUBLIC -g3)
  else()
    target_compile_options(Surge2048 PUBLIC /MP /MD)
  endif()

endif()

# -----------------------------------------
# Link and build order dependencies
# -----------------------------------------
target_link_libraries(Surge2048 PRIVATE glfw EASTL imgui::imgui)