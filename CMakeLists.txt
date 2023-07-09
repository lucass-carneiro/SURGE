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


include(${CMAKE_SOURCE_DIR}/cmake_components/find_packages.cmake)

# -----------------------------------------
# Options
# -----------------------------------------

include(${CMAKE_SOURCE_DIR}/cmake_components/add_options.cmake)

# -----------------------------------------
# Library targets
# -----------------------------------------

include(${CMAKE_SOURCE_DIR}/cmake_components/logging_system.cmake)
include(${CMAKE_SOURCE_DIR}/cmake_components/timer_system.cmake)

# -----------------------------------------
#  Main target
# -----------------------------------------

include(${CMAKE_SOURCE_DIR}/cmake_components/surge_target.cmake)


# -----------------------------------------
# Create stating directory
# -----------------------------------------

include(${CMAKE_SOURCE_DIR}/cmake_components/create_staging.cmake)
