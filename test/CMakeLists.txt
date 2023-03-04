set(SURGE_TESTS_SOURCE_LIST ${SURGE_SOURCE_LIST})
set(SURGE_TESTS_HEADER_LIST ${SURGE_HEADER_LIST})

list(REMOVE_ITEM SURGE_TESTS_SOURCE_LIST ${Surge_SOURCE_DIR}/source/main.cpp)

list(APPEND SURGE_TESTS_SOURCE_LIST "${Surge_SOURCE_DIR}/test/tests.cpp")
list(APPEND SURGE_TESTS_SOURCE_LIST "${Surge_SOURCE_DIR}/test/test_template.cpp")

enable_testing()

add_executable(tests ${SURGE_TESTS_HEADER_LIST} ${SURGE_TESTS_SOURCE_LIST})
target_compile_features(tests PRIVATE cxx_std_20)
set_target_properties(tests PROPERTIES OUTPUT_NAME "test")

if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
  target_compile_options(tests PUBLIC -O0 -g3 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer)
  target_link_options(tests PUBLIC -O0 -g3 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer)
else()
  # TODO:
endif()


target_include_directories(
    tests PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

target_link_libraries(tests PRIVATE
  OpenGL::GL
  EASTL
  fmt::fmt-header-only
  docopt_s
  tl::expected
  glfw
  glm::glm
  gsl::gsl-lite
  Taskflow::Taskflow
  PkgConfig::LUAJIT
  mimalloc-static
  imgui::imgui
  implot::implot
  GTest::gtest
  glad::glad
)

include(GoogleTest)
gtest_discover_tests(tests)