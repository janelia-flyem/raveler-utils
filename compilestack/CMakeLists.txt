set (CMAKE_CXX_FLAGS "-Wno-deprecated -Wall")
set (CMAKE_CXX_FLAGS_RELEASE "-O2")
set (CMAKE_CXX_FLAGS_DEBUG "-O0")
set (CMAKE_CXX_LINK_FLAGS "-lhdf5 -llibstack")
set (CMAKE_DEBUG_POSTFIX "-g")

include_directories (../libstack)
link_directories (${BUILDEM_LIB_DIR})
add_executable (compilestack compilestack.cpp)
add_dependencies (compilestack ${hdf5_NAME})

get_target_property (compilestack_exe compilestack LOCATION)
add_custom_command (
    TARGET compilestack
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${compilestack_exe} ${BUILDEM_DIR}/bin)

