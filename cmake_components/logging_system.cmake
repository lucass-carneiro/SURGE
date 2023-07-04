add_library(
  SurgeLoggingSystem
  SHARED
  "${Surge_SOURCE_DIR}/source/logging_system/logging_system.cpp"
)

target_link_libraries(SurgeLoggingSystem PRIVATE fmt::fmt-header-only spdlog::spdlog_header_only)

target_compile_features(SurgeLoggingSystem PRIVATE cxx_std_20)
set_target_properties(SurgeLoggingSystem PROPERTIES OUTPUT_NAME "surge_logging")
set_target_properties(SurgeLoggingSystem PROPERTIES PUBLIC_HEADER ${Surge_SOURCE_DIR}/include/logging_system/logging_system.hpp)

target_include_directories(
  SurgeLoggingSystem PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)