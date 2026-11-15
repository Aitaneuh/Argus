cmake_minimum_required(VERSION 3.16)
project(Argus C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_RELEASE "-O3")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(lib/chess-library)

# Fathom: Syzygy tablebase probing (tbprobe.c includes tbchess.c itself)
add_library(fathom STATIC lib/fathom/src/tbprobe.c)
target_include_directories(fathom PUBLIC lib/fathom/src)

file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(Argus ${SOURCES})
target_include_directories(Argus PRIVATE src/)
target_link_libraries(Argus PRIVATE fathom)

# Static runtime so Argus.exe runs from cutechess without needing MSYS2 DLLs on PATH
if(MINGW)
    target_link_options(Argus PRIVATE -static)
endif()

# Put the opening book next to the executable (Argus looks for it there by default)
add_custom_command(TARGET Argus POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/lib/opening-book/gm2001.bin $<TARGET_FILE_DIR:Argus>/gm2001.bin)

add_executable(Perft tests/perft.cpp)
target_include_directories(Perft PRIVATE lib/chess-library)
