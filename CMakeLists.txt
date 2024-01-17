cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# -----------------------------------------
# Vcpkg configuration
# -----------------------------------------

set(CMAKE_TOOLCHAIN_FILE vcpkg/scripts/buildsystems/vcpkg.cmake)
set(VCPKG_OVERLAY_TRIPLETS triplets/)

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

find_package(gsl-lite CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)
find_package(unofficial-inih CONFIG REQUIRED)
find_package(foonathan_memory CONFIG REQUIRED)
find_package(mimalloc CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(imgui REQUIRED)
find_package(Freetype REQUIRED)

# -----------------------------------------
# Operating system and compiler detection
# -----------------------------------------

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(SURGE_SYSTEM_IS_POSIX "IS_POSIX")
else()
  set(SURGE_SYSTEM_IS_POSIX "NOT_POSIX")
endif()

message(STATUS "Selected compiler: ${CMAKE_CXX_COMPILER_ID}")

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

# -----------------------------------------
# Threads
# -----------------------------------------

option(SURGE_MULTITHREADING "Enables parallelization" ON)

if(SURGE_MULTITHREADING)
  find_package(OpenMP REQUIRED)
endif()

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
  option(SURGE_ENABLE_HR "Enable module hot reloading when pressing LCTRL + F5" ON)
  option(SURGE_ENABLE_TRACY "Enables Tracy profiler annotations" OFF)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  option(SURGE_ENABLE_SANITIZERS "Compiles code with sanitizers" OFF)
  option(SURGE_ENABLE_OPTIMIZATIONS "Compiles code with optimizations" ON)
  option(SURGE_ENABLE_LTO "Compiles code with link time optimizations" ON)
  option(SURGE_ENABLE_FAST_MATH "Compiles code with fast math mode" ON)
  option(SURGE_ENABLE_TUNING "Compiles code with architecture tuning" ON)
  option(SURGE_DEBUG_MEMORY "Enable custom allocators debug facilities" OFF)
  option(SURGE_ENABLE_HR "Enable module hot reloading when pressing LCTRL + F5" OFF)
  option(SURGE_ENABLE_TRACY "Enables Tracy profiler annotations" OFF)
elseif(CMAKE_BUILD_TYPE STREQUAL "Profile")
  option(SURGE_ENABLE_SANITIZERS "Compiles code with sanitizers" OFF)
  option(SURGE_ENABLE_OPTIMIZATIONS "Compiles code with optimizations" ON)
  option(SURGE_ENABLE_LTO "Compiles code with link time optimizations" ON)
  option(SURGE_ENABLE_FAST_MATH "Compiles code with fast math mode" ON)
  option(SURGE_ENABLE_TUNING "Compiles code with architecture tuning" ON)
  option(SURGE_DEBUG_MEMORY "Enable custom allocators debug facilities" OFF)
  option(SURGE_ENABLE_HR "Enable module hot reloading when pressing LCTRL + F5" OFF)
  option(SURGE_ENABLE_TRACY "Enables Tracy profiler annotations" ON)
endif()

if(SURGE_ENABLE_TRACY)
  option(TRACY_ENABLE "" ON)
  option(TRACY_CALLSTACK "" ON)
  option(TRACY_STATIC "" OFF)
  option(TRACY_NO_EXIT "" OBJC_STANDARD_REQUIRED)

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    option(TRACY_LIBUNWIND_BACKTRACE "" ON)
    option(TRACY_LIBBACKTRACE_ELF_DYNLOAD_SUPPORT "" ON)
    add_compile_definitions(TRACY_DEBUGINFOD)
  endif()

  add_subdirectory(tracy)
endif()

# -----------------------------------------
# Buffer sizes
# -----------------------------------------

if(NOT DEFINED SURGE_OPENGL_ERROR_BUFFER_SIZE)
  set(SURGE_OPENGL_ERROR_BUFFER_SIZE 1024)
elseif(SURGE_OPENGL_ERROR_BUFFER_SIZE LESS 1024)
  message(WARNING "The variable SURGE_OPENGL_ERROR_BUFFER_SIZE should be set to at least 1024. It will be automatically set to this value.")
  set(SURGE_OPENGL_ERROR_BUFFER_SIZE 1024)
endif()

# -----------------------------------------
#  Engine player target
# -----------------------------------------

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/player)

# -----------------------------------------
#  Modules
# -----------------------------------------

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/modules)

# -----------------------------------------
#  Drizzle: The Unveiling
# -----------------------------------------

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/DTU)