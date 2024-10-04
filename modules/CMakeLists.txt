# -----------------------------------------
# Bundled Modules
# -----------------------------------------

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/default)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/sprite_demo)

# -----------------------------------------
# User Modules
# -----------------------------------------

# Gabriele Cirulli's 2048
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/SURGE-2048/CMakeLists.txt)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/SURGE-2048)
endif()

# Nguyễn Hà Đông's Flappy Bird
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/SURGE-FlappyBird/CMakeLists.txt)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/SURGE-FlappyBird/FlappyBird)
endif()

# Drizzle: The Unveiling
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/SURGE-DTU/CMakeLists.txt)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/SURGE-DTU/DTU)
endif()