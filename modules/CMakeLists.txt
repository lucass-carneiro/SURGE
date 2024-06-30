# -----------------------------------------
# Bundled Modules
# -----------------------------------------

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/default)

# -----------------------------------------
# User Modules
# -----------------------------------------

#  Gabriele Cirulli's 2048
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/SURGE-2048/CMakeLists.txt)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/SURGE-2048)
endif()


#  Nguyễn Hà Đông's Flappy Bird
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/SURGE-FlappyBird/CMakeLists.txt)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/SURGE-FlappyBird/FlappyBird)
endif()