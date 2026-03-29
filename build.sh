g++ -std=c++23 -I src -o build/out \
    src/main.cpp \
    src/lz77/lz77.cpp \
    src/huffman/huffman.cpp \
    src/deflate/bitstream.cpp \
    src/deflate/deflate.cpp
