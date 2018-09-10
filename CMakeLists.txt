cmake_minimum_required(VERSION 3.0.0)
project(http VERSION 0.1.0)

include(CTest)
enable_testing()
set (CMAKE_CXX_STANDARD 11)

#add sub dircetory
add_subdirectory(libevhtp)
add_subdirectory(cjson)

aux_source_directory(. DIR_SRCS)
#aux_source_directory(libevhtp DIR_SRCS)

add_executable(http ${DIR_SRCS})

target_link_libraries(http libevhtp cjson event)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
