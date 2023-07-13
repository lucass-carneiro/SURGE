# -----------------------------------------
#  Main target sources
# -----------------------------------------

set(
  SURGE_HEADER_LIST
  "${Surge_SOURCE_DIR}/include/entities/actor.hpp"
  "${Surge_SOURCE_DIR}/include/entities/animated_sprite.hpp"
  "${Surge_SOURCE_DIR}/include/entities/image.hpp"

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
  "${Surge_SOURCE_DIR}/include/window.hpp"
)

set(
  SURGE_SOURCE_LIST
  "${Surge_SOURCE_DIR}/source/entities/actor.cpp"
  "${Surge_SOURCE_DIR}/source/entities/animated_sprite.cpp"
  "${Surge_SOURCE_DIR}/source/entities/image.cpp"

  "${Surge_SOURCE_DIR}/source/lua/lua_actor_wrappers.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_image_wrappers.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_logs.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_callback_wrappers.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_geometry_wrappers.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_get_engine_config.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_add_engine_context.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_animated_sprite_wrappers.cpp"
  "${Surge_SOURCE_DIR}/source/lua/lua_states.cpp"
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
# Link and build order dependencies
# -----------------------------------------

add_dependencies(Surge
  SurgeLoggingSystem
  SurgeTimerSystem
)

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
  EASTL
  SurgeLoggingSystem # On Windows, these lines should be removed. This is only used for static linking there.
  SurgeTimerSystem # On Windows, these lines should be removed. This is only used for static linking there.
)