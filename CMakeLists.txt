cmake_minimum_required(VERSION 3.20)
project(compression)

set(CMAKE_CXX_STANDARD 23)

# Static library with all compression algorithms
add_library(compresslib STATIC
    src/lz77/lz77.cpp
    src/huffman/huffman.cpp
    src/deflate/bitstream.cpp
    src/deflate/deflate.cpp
    src/gzip/gzip.cpp
)
target_include_directories(compresslib PUBLIC src)

# Demo executable
add_executable(compression src/main.cpp)
target_link_libraries(compression PRIVATE compresslib)

# CLI tool
add_executable(cpgz src/cli/cpgz.cpp)
target_link_libraries(cpgz PRIVATE compresslib)
