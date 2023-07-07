cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# -----------------------------------------
# Vcpkg configuration
# -----------------------------------------

set(CMAKE_TOOLCHAIN_FILE vcpkg/scripts/buildsystems/vcpkg.cmake)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  Surge
  VERSION 1.0.0
  LANGUAGES CXX
)

# Generate compile_commands.json for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

# -----------------------------------------
# In-source build guard
# -----------------------------------------

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
    "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# -----------------------------------------
# External dependencies
# -----------------------------------------

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
list(APPEND VCPKG_FEATURE_FLAGS "versions")

find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)

find_package(mimalloc CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(gsl-lite CONFIG REQUIRED)
find_package(Taskflow CONFIG REQUIRED)
pkg_search_module(LUAJIT REQUIRED IMPORTED_TARGET luajit)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(EASTL CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# -----------------------------------------
# Size options
# -----------------------------------------

if(NOT DEFINED SURGE_OPENGL_ERROR_BUFFER_SIZE)
  set(SURGE_OPENGL_ERROR_BUFFER_SIZE 1024)
elseif(SURGE_OPENGL_ERROR_BUFFER_SIZE LESS 1024)
  message(WARNING "The variable SURGE_OPENGL_ERROR_BUFFER_SIZE should be set to at least 1024. It will be automatically set to this value.")
  set(SURGE_OPENGL_ERROR_BUFFER_SIZE 1024)
endif()

if(NOT DEFINED SURGE_FPS_COUNTER_SAMPLE_SIZE)
  set(SURGE_FPS_COUNTER_SAMPLE_SIZE 1024)
endif()

# -----------------------------------------
# Operating system and compiler detection
# -----------------------------------------

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(SURGE_SYSTEM_IS_POSIX "IS_POSIX")
else()
  set(SURGE_SYSTEM_IS_POSIX "NOT_POSIX")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
  set(SURGE_COMPILER_FLAG_STYLE "gcc")
else()
  set(SURGE_COMPILER_FLAG_STYLE "msvc")
endif()

# -----------------------------------------
# Logging options
# -----------------------------------------

option(SURGE_USE_LOG "Enable log messages" ON)
option(SURGE_USE_LOG_COLOR "Use colors on log outputs" ON)
option(SURGE_STBIMAGE_ERRORS "Enables more verbose error message strings in stb_image" ON)

# -----------------------------------------
# Compilation flag options
# -----------------------------------------

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  option(SURGE_ENABLE_SANITIZERS "Compiles code with sanitizers" ON)
  option(SURGE_ENABLE_OPTIMIZATIONS "Compiles code with optimizations" OFF)
  option(SURGE_ENABLE_LTO "Compiles code with link time optimizations" OFF)
  option(SURGE_ENABLE_FAST_MATH "Compiles code with fast math mode" OFF)
  option(SURGE_ENABLE_TUNING "Compiles code with architecture tuning" OFF)
option(SURGE_DEBUG_MEMORY "Enable custom allocators debug facilities" ON)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  option(SURGE_ENABLE_SANITIZERS "Compiles code with sanitizers" OFF)
  option(SURGE_ENABLE_OPTIMIZATIONS "Compiles code with optimizations" ON)
  option(SURGE_ENABLE_LTO "Compiles code with link time optimizations" ON)
  option(SURGE_ENABLE_FAST_MATH "Compiles code with fast math mode" ON)
  option(SURGE_ENABLE_TUNING "Compiles code with architecture tuning" ON)
  option(SURGE_DEBUG_MEMORY "Enable custom allocators debug facilities" OFF)
endif()

# -----------------------------------------
# Other options
# -----------------------------------------

option(SURGE_ENABLE_THREADS "Enables multithreading" ON)
option(SURGE_ENABLE_TRACY "Eneblas profiling via Tacy" OFF)

if(SURGE_ENABLE_TRACY)
  option(TRACY_ENABLE "Enables tracy profiling" ON)
else()
  option(TRACY_ENABLE "Enables tracy profiling" OFF)
endif()

# -----------------------------------------
# Option file
# -----------------------------------------

configure_file("${Surge_SOURCE_DIR}/include/options_in.txt" "${Surge_SOURCE_DIR}/include/options.hpp")

# -----------------------------------------
#  Main Source files
# -----------------------------------------

set(
  SURGE_HEADER_LIST
  "${Surge_SOURCE_DIR}/include/entities/actor.hpp"
  "${Surge_SOURCE_DIR}/include/entities/animated_sprite.hpp"
  "${Surge_SOURCE_DIR}/include/entities/image.hpp"

  "${Surge_SOURCE_DIR}/include/gui_windows/gui_windows.hpp"

  "${Surge_SOURCE_DIR}/include/lua/lua_bindings.hpp"
  "${Surge_SOURCE_DIR}/include/lua/lua_logs.hpp"
  "${Surge_SOURCE_DIR}/include/lua/lua_states.hpp"
  "${Surge_SOURCE_DIR}/include/lua/lua_utils.hpp"
  "${Surge_SOURCE_DIR}/include/lua/lua_wrappers.hpp"

  "${Surge_SOURCE_DIR}/include/opengl/buffer_usage_hints.hpp"
  "${Surge_SOURCE_DIR}/include/opengl/glm.hpp"
  "${Surge_SOURCE_DIR}/include/opengl/headers.hpp"
  "${Surge_SOURCE_DIR}/include/opengl/load_texture.hpp"
  "${Surge_SOURCE_DIR}/include/opengl/program.hpp"
  "${Surge_SOURCE_DIR}/include/opengl/uniforms.hpp"

  "${Surge_SOURCE_DIR}/include/stb/stb_image.hpp"
  
  "${Surge_SOURCE_DIR}/include/allocator.hpp"
  "${Surge_SOURCE_DIR}/include/cli.hpp"
  "${Surge_SOURCE_DIR}/include/file.hpp"
  "${Surge_SOURCE_DIR}/include/geometry_utils.hpp"
  "${Surge_SOURCE_DIR}/include/image_loader.hpp"
  "${Surge_SOURCE_DIR}/include/options.hpp"
  "${Surge_SOURCE_DIR}/include/profile_mimalloc.h"
  "${Surge_SOURCE_DIR}/include/sad_file.hpp"
  "${Surge_SOURCE_DIR}/include/safe_ops.hpp"
  "${Surge_SOURCE_DIR}/include/static_map.hpp"
  "${Surge_SOURCE_DIR}/include/task_executor.hpp"
  "${Surge_SOURCE_DIR}/include/window.hpp"
)

set(
  SURGE_SOURCE_LIST
  "${Surge_SOURCE_DIR}/source/entities/actor.cpp"
  "${Surge_SOURCE_DIR}/source/entities/animated_sprite.cpp"
  "${Surge_SOURCE_DIR}/source/entities/image.cpp"

  "${Surge_SOURCE_DIR}/source/gui_windows/fps_counter_window.cpp"
  "${Surge_SOURCE_DIR}/source/gui_windows/main_gui_window.cpp"

  "${Surge_SOURCE_DIR}/source/lua/lua_actor_wrappers.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_image_wrappers.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_logs.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_callback_wrappers.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_geometry_wrappers.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_get_engine_config.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_add_engine_context.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_animated_sprite_wrappers.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_states.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_task_wrappers.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_util_wrappers.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_utils.cpp"

  "${Surge_SOURCE_DIR}/source/opengl/program.cpp"
  "${Surge_SOURCE_DIR}/source/opengl/uniforms.cpp"
  "${Surge_SOURCE_DIR}/source/opengl/load_texture.cpp"

  "${Surge_SOURCE_DIR}/source/stb/stb_image_impl.cpp"

  "${Surge_SOURCE_DIR}/source/allocator.cpp"
  "${Surge_SOURCE_DIR}/source/cli.cpp"
  "${Surge_SOURCE_DIR}/source/file.cpp"
  "${Surge_SOURCE_DIR}/source/geometry_utils.cpp"
  "${Surge_SOURCE_DIR}/source/image_loader.cpp"
  "${Surge_SOURCE_DIR}/source/main.cpp"
  "${Surge_SOURCE_DIR}/source/sad_file.cpp"
  "${Surge_SOURCE_DIR}/source/window.cpp"
)

# -----------------------------------------
# Library targets
# -----------------------------------------

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake_components/logging_system.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake_components/timer_system.cmake)

# -----------------------------------------
# Executable engine target
# -----------------------------------------

add_executable(Surge ${SURGE_HEADER_LIST} ${SURGE_SOURCE_LIST})
target_compile_features(Surge PRIVATE cxx_std_20)
set_target_properties(Surge PROPERTIES OUTPUT_NAME "surge")

target_include_directories(
  Surge PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

target_compile_definitions(Surge PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLEW)

if(SURGE_ENABLE_SANITIZERS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge PUBLIC -fsanitize=address,null,unreachable,undefined)
    target_link_options(Surge PUBLIC -fsanitize=address,null,unreachable,undefined)
  else()
    # TODO
  endif()
endif()

if(SURGE_ENABLE_OPTIMIZATIONS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge PUBLIC -O2)
    target_link_options(Surge PUBLIC -O2)
  else()
    target_compile_options(Surge PUBLIC /O2)
  endif()
endif()

if(SURGE_ENABLE_TUNING)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge PUBLIC -march=native -mtune=native)
    target_link_options(Surge PUBLIC -march=native -mtune=native)
  else()
    # TODO
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge PUBLIC -flto)
    target_link_options(Surge PUBLIC -flto)
  else()
    # TODO
  endif()
endif()

if(SURGE_ENABLE_FAST_MATH)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge PUBLIC -ffast-math)
    target_link_options(Surge PUBLIC -ffast-math)
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
    target_compile_options(Surge PUBLIC -O0 -g3 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer)
    target_link_options(Surge PUBLIC -O0 -g3 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer)
  else()
    target_compile_options(Surge PUBLIC /MP /MDd)
  endif()

endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "Generating a Release build system")

  if(SURGE_ENABLE_TRACY)
    find_package(Tracy CONFIG REQUIRED)
    target_link_libraries(Surge PRIVATE Tracy::TracyClient)
  endif()

  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(Surge PUBLIC -g3)
    target_link_options(Surge PUBLIC -g3)
  else()
    target_compile_options(Surge PUBLIC /MP /MD)
  endif()

endif()

# -----------------------------------------
# Link dependencies
# -----------------------------------------

target_link_libraries(Surge PRIVATE
  OpenGL::GL
  mimalloc-static  
  fmt::fmt-header-only
  gsl::gsl-lite
  Taskflow::Taskflow
  PkgConfig::LUAJIT
  glm::glm
  glad::glad
  glfw
  imgui::imgui
  implot::implot
  EASTL
  spdlog::spdlog_header_only
  SurgeLoggingSystem # On Windows, these lines should be removed. This is only used for static linking there.
  SurgeTimerSystem # On Windows, these lines should be removed. This is only used for static linking there.
)