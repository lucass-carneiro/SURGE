set(SURGE_TESTS_SOURCE_LIST ${SURGE_SOURCE_LIST})
set(SURGE_TESTS_HEADER_LIST ${SURGE_HEADER_LIST})

list(REMOVE_ITEM SURGE_TESTS_SOURCE_LIST ${Surge_SOURCE_DIR}/source/main.cpp)

list(APPEND SURGE_TESTS_SOURCE_LIST "${Surge_SOURCE_DIR}/test/tests.cpp")
list(APPEND SURGE_TESTS_SOURCE_LIST "${Surge_SOURCE_DIR}/test/test_template.cpp")

enable_testing()

add_executable(tests ${SURGE_TESTS_HEADER_LIST} ${SURGE_TESTS_SOURCE_LIST})
target_compile_features(tests PRIVATE cxx_std_20)
set_target_properties(tests PROPERTIES OUTPUT_NAME "test")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(SURGE_COMPILER_FLAG_STYLE MATCHES "gcc")
    target_compile_options(tests PUBLIC -g3 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer)
    target_link_options(tests PUBLIC -g3 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer)
  else()
    # TODO:
  endif()
endif()


target_include_directories(
    tests PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

target_link_libraries(tests PRIVATE
  OpenGL::GL
  CONAN_PKG::glad
  CONAN_PKG::eastl
  CONAN_PKG::fmt
  CONAN_PKG::docopt.cpp
  CONAN_PKG::tl-expected
  CONAN_PKG::gtest
  CONAN_PKG::glfw
  CONAN_PKG::glm
  CONAN_PKG::gsl-lite
  CONAN_PKG::mimalloc
  CONAN_PKG::taskflow
  CONAN_PKG::luajit
  CONAN_PKG::imgui
  CONAN_PKG::implot
)

include(GoogleTest)
gtest_discover_tests(tests)