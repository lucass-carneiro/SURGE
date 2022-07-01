set(SURGE_TESTS_SOURCE_LIST ${SURGE_SOURCE_LIST})
set(SURGE_TESTS_HEADER_LIST ${SURGE_HEADER_LIST})

list(REMOVE_ITEM SURGE_TESTS_SOURCE_LIST ${Surge_SOURCE_DIR}/source/main.cpp)

list(APPEND SURGE_TESTS_SOURCE_LIST "${Surge_SOURCE_DIR}/test/tests.cpp")
list(APPEND SURGE_TESTS_SOURCE_LIST "${Surge_SOURCE_DIR}/test/test_template.cpp")

#list(APPEND SURGE_TESTS_HEADER_LIST "${Surge_SOURCE_DIR}/test/HEADER_NAME.hpp")

enable_testing()

add_executable(tests ${SURGE_TESTS_HEADER_LIST} ${SURGE_TESTS_SOURCE_LIST})
target_compile_features(tests PRIVATE cxx_std_20)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    target_compile_options(Surge PUBLIC -g3 -O0 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer -fsanitize=undefined)
    target_link_options(Surge PUBLIC -g3 -O0 -Wall -Wextra -Werror -pedantic -pedantic-errors -fno-omit-frame-pointer -fsanitize=undefined)
endif()

target_include_directories(
    tests PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

target_link_libraries(tests PRIVATE 
  CONAN_PKG::eastl
  CONAN_PKG::fmt
  CONAN_PKG::docopt.cpp
  CONAN_PKG::tl-expected
  CONAN_PKG::gtest
  CONAN_PKG::squirrel
  CONAN_PKG::glfw
  CONAN_PKG::glm
  CONAN_PKG::gsl-lite
  CONAN_PKG::date
)

include(GoogleTest)
gtest_discover_tests(tests)