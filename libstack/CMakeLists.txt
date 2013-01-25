set (SOURCES libstack.cpp HdfStack.cpp HdfFile.cpp Table.cpp timers.cpp util.cpp LogFile.cpp 
             BodyColorTable.cpp STLExport.cpp)

set (CMAKE_CXX_FLAGS "-Wno-deprecated -Wall -fPIC")
set (CMAKE_CXX_FLAGS_RELEASE "-O2")
set (CMAKE_CXX_FLAGS_DEBUG "-O0")
set (CMAKE_CXX_LINK_FLAGS "-lhdf5")
set (CMAKE_DEBUG_POSTFIX "-g")

link_directories (${BUILDEM_LIB_DIR})

add_library (libstack SHARED ${SOURCES})
add_dependencies (libstack ${hdf5_NAME})

get_target_property (libstack_lib libstack LOCATION)

add_custom_command (
    TARGET libstack
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILDEM_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E copy ${libstack_lib} ${BUILDEM_DIR}/lib)

