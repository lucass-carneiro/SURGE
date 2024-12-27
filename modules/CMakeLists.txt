file(GLOB SUBDIRS *)

foreach(subdir ${SUBDIRS})
    if(IS_DIRECTORY ${subdir} AND EXISTS "${subdir}/CMakeLists.txt")
        add_subdirectory(${subdir})
    endif()
endforeach()