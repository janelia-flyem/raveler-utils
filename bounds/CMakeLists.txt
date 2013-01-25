set (SOURCES bounds.cpp PngImage.cpp Stack.cpp PixelBoundBox.cpp)

set (CMAKE_CXX_FLAGS "-Wno-deprecated -Wall")
set (CMAKE_CXX_FLAGS_RELEASE "-O2")
set (CMAKE_CXX_FLAGS_DEBUG "-O0")
set (CMAKE_CXX_LINK_FLAGS "-lpng")
set (CMAKE_DEBUG_POSTFIX "-g")

link_directories (${BUILDEM_LIB_DIR})
add_executable (bounds ${SOURCES})
add_dependencies (bounds ${libpng_NAME})

get_target_property (bounds_exe bounds LOCATION)
add_custom_command (
    TARGET bounds
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${bounds_exe} ${BUILDEM_DIR}/bin)

