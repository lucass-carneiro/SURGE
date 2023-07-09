# -----------------------------------------
# Buffer sizes
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