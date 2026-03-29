cmake_minimum_required(VERSION 3.20)
project(compression)

set(CMAKE_CXX_STANDARD 23)

set(SOURCES
    src/main.cpp
    src/lz77/lz77.cpp
    src/huffman/huffman.cpp
    src/deflate/bitstream.cpp
    src/deflate/deflate.cpp
)

add_executable(compression ${SOURCES})
target_include_directories(compression PRIVATE src)
