cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# -----------------------------------------
# Project
# -----------------------------------------

project(
  SurgeCoreShaders
  VERSION 1.3.0
  LANGUAGES CXX
)

# -----------------------------------------
#  Target sources
# -----------------------------------------

set(
  SURGE_CORE_SHADER_LIST
  "${PROJECT_LIST_DIR}/vk/compute_demo.comp"
)

# -----------------------------------------
#  Compilation
# -----------------------------------------

foreach(GLSL ${SURGE_CORE_SHADER_LIST})
  message(STATUS "Building shader ${GLSL}")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  get_filename_component(FILE_DIR ${GLSL} DIRECTORY)
  set(SPIRV "${FILE_DIR}/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL}
  )
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)