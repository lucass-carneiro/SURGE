set(SURGE_STAGING_DIR "${CMAKE_SOURCE_DIR}/staging_${CMAKE_BUILD_TYPE}")

add_custom_target(
  SurgeStatingDirectory
  ALL
  DEPENDS Surge
  COMMAND ${CMAKE_COMMAND} -E echo "Creating staging directory for ${CMAKE_BUILD_TYPE} build"
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SURGE_STAGING_DIR}
  COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:Surge> ${SURGE_STAGING_DIR}/$<TARGET_FILE_NAME:Surge>
  COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:SurgeLoggingSystem> ${SURGE_STAGING_DIR}/$<TARGET_FILE_NAME:SurgeLoggingSystem>
  COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:SurgeTimerSystem> ${SURGE_STAGING_DIR}/$<TARGET_FILE_NAME:SurgeTimerSystem>
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/shaders ${SURGE_STAGING_DIR}/shaders
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/core_scripts ${SURGE_STAGING_DIR}/core_scripts
  VERBATIM  
)