cmake_minimum_required(VERSION 3.20)
project (compression)

set(CMAKE_CXX_STANDARD 23)

set(SOURCES src/main.cpp src/lz77/lz77.cpp)

add_executable(compression ${SOURCES})