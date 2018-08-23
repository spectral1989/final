
set (CMAKE_CXX_STANDARD 11)
add_executable(final final.cpp http_parser.c)


target_link_libraries(final pthread)
