add_library(
  SurgeTimerSystem
  SHARED
  "${Surge_SOURCE_DIR}/source/timer_system/timer_system.cpp"
)

target_compile_features(SurgeTimerSystem PRIVATE cxx_std_20)
set_target_properties(SurgeTimerSystem PROPERTIES OUTPUT_NAME "surge_timer")
set_target_properties(SurgeTimerSystem PROPERTIES PUBLIC_HEADER ${Surge_SOURCE_DIR}/include/timer_system/timer_system.hpp)

target_include_directories(
  SurgeTimerSystem PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)