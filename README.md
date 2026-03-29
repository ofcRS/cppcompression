# Compression Algorithms in C++23

Implementations of classic compression algorithms from scratch, using modern C++23.

## Algorithms

### LZ77

Sliding-window dictionary compression. Encodes repeated byte sequences as `(offset, length, next_char)` triples. Configurable window size (default 4096).

### DEFLATE (RFC 1951)

Combines LZ77 with Huffman coding. Supports all three block types:
- **Type 0** — Stored (uncompressed)
- **Type 1** — Fixed Huffman codes
- **Type 2** — Dynamic Huffman codes (optimized per block)

Output is compliant with RFC 1951 and can be decompressed by zlib/gzip.

### Huffman Coding

Canonical Huffman tree construction from symbol frequencies. Used internally by DEFLATE, also available as a standalone module.

## Build

```
mkdir -p build
g++ -std=c++23 -I src -o build/out \
    src/main.cpp \
    src/lz77/lz77.cpp \
    src/huffman/huffman.cpp \
    src/deflate/bitstream.cpp \
    src/deflate/deflate.cpp
./build/out
```

Or with CMake:

```
cmake -B build && cmake --build build
./build/compression
```

## Project structure

```
src/
  main.cpp                       — demo and round-trip tests
  lz77/lz77.h, lz77.cpp         — standalone LZ77
  huffman/huffman.h, huffman.cpp — canonical Huffman coding
  deflate/deflate.h, deflate.cpp — DEFLATE compress/decompress
  deflate/bitstream.h, .cpp      — bit-level I/O (LSB-first)
```
