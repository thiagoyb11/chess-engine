cmake_minimum_required(VERSION 3.20)

project(chess_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CHESS_ENGINE_NATIVE_ARCH "Optimize for the build machine's CPU" OFF)

set(CHESS_ENGINE_CORE_SOURCES
    src/board.cpp
    src/magic_bitboards.cpp
    src/move_generation.cpp
    src/zobrist_hashing.cpp
)

add_executable(chess-engine
    src/main.cpp
    src/search.cpp
    ${CHESS_ENGINE_CORE_SOURCES}
)

add_executable(chess-engine-perft
    src/perft-testing.cpp
    ${CHESS_ENGINE_CORE_SOURCES}
)

foreach(target chess-engine chess-engine-perft)
    target_include_directories(${target} PRIVATE src)
    target_compile_features(${target} PRIVATE cxx_std_17)

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(CHESS_ENGINE_NATIVE_ARCH)
            target_compile_options(${target} PRIVATE -march=native -mtune=native)
        endif()
    endif()
endforeach()
