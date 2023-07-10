find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)

find_package(mimalloc CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(gsl-lite CONFIG REQUIRED)
find_package(Taskflow CONFIG REQUIRED)
pkg_search_module(LUAJIT REQUIRED IMPORTED_TARGET luajit)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(EASTL CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)