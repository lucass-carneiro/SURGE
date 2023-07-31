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

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
list(APPEND VCPKG_FEATURE_FLAGS "versions")

find_package (Python3 COMPONENTS Interpreter)

find_package(mimalloc CONFIG REQUIRED)
find_package(gsl-lite CONFIG REQUIRED)
find_package(EASTL CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(bgfx CONFIG REQUIRED)

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
#  Engine player target
# -----------------------------------------

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/surge_player)


# -----------------------------------------
#  Modules
# -----------------------------------------

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/surge_modules)

# -----------------------------------------
# Create staging directory
# -----------------------------------------

# We need to make sure that this script runs after all targets are built
# https://stackoverflow.com/a/62311397
function(get_all_targets var)
    set(targets)
    get_all_targets_recursive(targets ${CMAKE_CURRENT_SOURCE_DIR})
    set(${var} ${targets} PARENT_SCOPE)
endfunction()

macro(get_all_targets_recursive targets dir)
    get_property(subdirectories DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
    foreach(subdir ${subdirectories})
        get_all_targets_recursive(${targets} ${subdir})
    endforeach()

    get_property(current_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    list(APPEND ${targets} ${current_targets})
endmacro()

get_all_targets(all_targets)

add_custom_target(
  SurgeStatingDirectory
  ALL
  DEPENDS ${all_targets}
  COMMAND ${CMAKE_COMMAND} -E echo "Creating ${CMAKE_BUILD_TYPE} staging for SurgePlayer"
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/make_staging.py ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_BUILD_TYPE}
  VERBATIM  
)
