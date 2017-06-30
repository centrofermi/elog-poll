project(elog-parse CXX)
cmake_minimum_required(VERSION 2.6)

add_library(elog src/libelog.cpp)

add_executable(elog-parse src/elog-parse.cpp)
target_link_libraries(elog-parse elog)
