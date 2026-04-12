cmake_minimum_required(VERSION 3.16)
project(Chessy)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(lib/chess-library)

file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(Chessy ${SOURCES})