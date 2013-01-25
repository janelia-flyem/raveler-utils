set (CMAKE_CXX_FLAGS "-Wno-deprecated -Wall")
set (CMAKE_CXX_FLAGS_RELEASE "-O2")
set (CMAKE_CXX_FLAGS_DEBUG "-O0")
set (CMAKE_CXX_LINK_FLAGS "-lhdf5 -llibstack")
set (CMAKE_DEBUG_POSTFIX "-g")

include_directories (../libstack)
link_directories (${BUILDEM_LIB_DIR})
add_executable (verifystack verifystack.cpp)
add_dependencies (verifystack ${hdf5_NAME})

get_target_property (verifystack_exe verifystack LOCATION)
add_custom_command (
    TARGET verifystack
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${verifystack_exe} ${BUILDEM_DIR}/bin)

