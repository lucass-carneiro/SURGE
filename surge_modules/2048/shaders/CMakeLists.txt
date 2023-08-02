# -----------------------------------------
#  Target sources
# -----------------------------------------

set(
  SURGE_MODULE_2048_SHADER_FILES
  "${CMAKE_CURRENT_LIST_DIR}/src/image.frag"
  "${CMAKE_CURRENT_LIST_DIR}/src/image.vert"
  "${CMAKE_CURRENT_LIST_DIR}/src/varying.def.sc"
)

# -----------------------------------------
#  Invoke shader compiler
# -----------------------------------------

add_custom_target(
  SurgeModule2048Shaders
  ALL
  DEPENDS SurgeModule2048 ${SURGE_MODULE_2048_SHADER_FILES}
  COMMAND ${CMAKE_COMMAND} -E echo "Compiling SurgeModule2048 shaders"
  COMMAND ${Python3_EXECUTABLE} ${Surge_SOURCE_DIR}/compile_shaders.py ${CMAKE_CURRENT_LIST_DIR} ${BGFX_SHADERC_EXECUTABLE}
  VERBATIM
)