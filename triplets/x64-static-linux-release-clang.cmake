# See https://learn.microsoft.com/en-us/vcpkg/users/triplets for documentation
set(VCPKG_TARGET_ARCHITECTURE x64)

set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_BUILD_TYPE release)
set(VCPKG_CMAKE_CONFIGURE_OPTIONS "-DCMAKE_TRY_COMPILE_CONFIGURATION=Release")

set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/x64-linux-release-clang.toolchain.cmake")
