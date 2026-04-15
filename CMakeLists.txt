cmake_minimum_required(VERSION 3.16)
project(Argus)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(lib/chess-library)

file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(Argus ${SOURCES})
target_include_directories(Argus PRIVATE src/)

add_executable(Perft tests/perft.cpp)
target_include_directories(Perft PRIVATE lib/chess-library)