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
  VERSION 1.1.0
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

find_package(mimalloc CONFIG REQUIRED)
find_package(foonathan_memory CONFIG REQUIRED)
find_package(gsl-lite CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Taskflow CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(OpenEXR CONFIG REQUIRED)
find_package(ryml CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED)

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
  if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
    endif()
endif()

# -----------------------------------------
# Logging options
# -----------------------------------------

option(SURGE_USE_LOG "Enable log messages" ON)
option(SURGE_USE_LOG_COLOR "Use colors on log outputs" ON)
option(SURGE_LOG_GL_NOTIFICATIONS "Produce log outputs on GL_DEBUG_SEVERITY_NOTIFICATION events" ON)

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
#  Engine core target
# -----------------------------------------

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/core)

# -----------------------------------------
#  Engine player target
# -----------------------------------------

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/player)

# -----------------------------------------
#  Module targets
# -----------------------------------------

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/modules)