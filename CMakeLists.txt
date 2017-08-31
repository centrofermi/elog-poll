project(elog-parse CXX)
cmake_minimum_required(VERSION 2.6)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)

if (COMPILER_SUPPORTS_CXX17)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
elseif (COMPILER_SUPPORTS_CXX14)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
elseif (COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else ()
  message(
    FATAL_ERROR
    "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler."
  )
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")

find_package(ROOT 5.32.00 REQUIRED)

# ROOT include dirs
include_directories(${ROOT_INCLUDE_DIR})

add_library(elog src/libelog.cpp)

# Parser
add_executable(elog-parse src/elog-parse.cpp)
target_link_libraries(elog-parse elog)

# Producer
add_executable(producer src/producer.cpp)
target_link_libraries(producer ${ROOT_LIBRARIES})
