# -----------------------------------------
# Logging library target
# -----------------------------------------

add_library(SurgeLoggingSystem SHARED
  "${Surge_SOURCE_DIR}/source/logging_system/logging_system.cpp"
)

target_compile_features(SurgeLoggingSystem PRIVATE cxx_std_20)
set_target_properties(SurgeLoggingSystem PROPERTIES OUTPUT_NAME "surge_logging")

set_target_properties(SurgeLoggingSystem PROPERTIES PUBLIC_HEADER
  ${Surge_SOURCE_DIR}/include/logging_system/logging_system.hpp
)

target_include_directories(
  SurgeLoggingSystem PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# -----------------------------------------
# Compilers flags and options
# -----------------------------------------

if(SURGE_ENABLE_SANITIZERS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeLoggingSystem PUBLIC -fsanitize=address,null,unreachable,undefined)
    target_link_options(SurgeLoggingSystem PUBLIC -fsanitize=address,null,unreachable,undefined)
  else()
    # TODO
  endif()
endif()

if(SURGE_ENABLE_OPTIMIZATIONS)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeLoggingSystem PUBLIC -O2)
    target_link_options(SurgeLoggingSystem PUBLIC -O2)
  else()
    target_compile_options(SurgeLoggingSystem PUBLIC /O2)
  endif()
endif()

if(SURGE_ENABLE_TUNING)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeLoggingSystem PUBLIC -march=native -mtune=native)
    target_link_options(SurgeLoggingSystem PUBLIC -march=native -mtune=native)
  else()
    # TODO
  endif()
endif()

if(SURGE_ENABLE_LTO)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeLoggingSystem PUBLIC -flto)
    target_link_options(SurgeLoggingSystem PUBLIC -flto)
  else()
    # TODO
  endif()
endif()

if(SURGE_ENABLE_FAST_MATH)
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeLoggingSystem PUBLIC -ffast-math)
    target_link_options(SurgeLoggingSystem PUBLIC -ffast-math)
  else()
    # TODO
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Generating a Debug build system")

  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeLoggingSystem PUBLIC -O0 -g3 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer)
    target_link_options(SurgeLoggingSystem PUBLIC -O0 -g3 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer)
  else()
    target_compile_options(SurgeLoggingSystem PUBLIC /MP /MDd)
  endif()

endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "Generating a Release build system")

  if(SURGE_ENABLE_TRACY)
    find_package(Tracy CONFIG REQUIRED)
    target_link_libraries(SurgeLoggingSystem PRIVATE Tracy::TracyClient)
  endif()

  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(SurgeLoggingSystem PUBLIC -g3)
    target_link_options(SurgeLoggingSystem PUBLIC -g3)
  else()
    target_compile_options(SurgeLoggingSystem PUBLIC /MP /MD)
  endif()

endif()

# -----------------------------------------
# Link dependencies
# -----------------------------------------

target_link_libraries(SurgeLoggingSystem PRIVATE
  fmt::fmt-header-only
  spdlog::spdlog_header_only
)