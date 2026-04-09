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

### GZIP (RFC 1952)

Wraps DEFLATE output in the gzip container format:
- 10-byte header (magic bytes, compression method, flags, MTIME, OS)
- Parses optional header fields (FEXTRA, FNAME, FCOMMENT, FHCRC)
- 8-byte trailer with CRC-32 checksum and original size
- Output is compatible with standard `gunzip`

### Huffman Coding

Canonical Huffman tree construction from symbol frequencies. Used internally by DEFLATE, also available as a standalone module.

## CLI Tool

`cpgz` compresses and decompresses files in gzip format, compatible with standard `gunzip`.

```
cpgz file.txt              # compress → file.txt.gz, removes original
cpgz -d file.txt.gz        # decompress → file.txt, removes original
cpgz -k file.txt           # compress, keep original
cpgz -f file.txt           # overwrite existing output
cpgz -c file.txt | gunzip  # compress to stdout (pipe-friendly)
cpgz -vk file.txt          # verbose: print compression ratio
```

Options: `-d` decompress, `-k` keep, `-f` force, `-c` stdout, `-v` verbose, `-h` help.

## Build

```
cmake -B build && cmake --build build
```

This produces two executables:
- `./build/compression` — demo and round-trip tests
- `./build/cpgz` — CLI tool

## Project structure

```
src/
  main.cpp                       — demo and round-trip tests
  lz77/lz77.h, lz77.cpp         — standalone LZ77
  huffman/huffman.h, huffman.cpp — canonical Huffman coding
  deflate/deflate.h, deflate.cpp — DEFLATE compress/decompress
  deflate/bitstream.h, .cpp      — bit-level I/O (LSB-first)
  gzip/gzip.h, gzip.cpp         — GZIP format with CRC-32
  cli/cpgz.cpp                   — command-line tool
```
