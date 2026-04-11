#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace compression {

// Computes CRC-32 checksum (ISO 3309 / ITU-T V.42 polynomial).
[[nodiscard]] uint32_t crc32(std::span<const uint8_t> data);

// Compresses data into gzip format (RFC 1952).
// The output is compatible with standard gunzip / zlib.
[[nodiscard]] std::vector<uint8_t> gzip_compress(std::span<const uint8_t> input);

// Decompresses gzip-formatted data (RFC 1952).
[[nodiscard]] std::vector<uint8_t> gzip_decompress(std::span<const uint8_t> input);

// Metadata extracted from a gzip stream without decompressing the payload.
// `uncompressed_size` and `crc32` come from the 8-byte trailer (ISIZE is mod 2^32).
// `original_name` is populated only when the FNAME header flag is set.
struct GzipInfo {
    uint32_t mtime;             // seconds since epoch, 0 if not set
    uint32_t uncompressed_size; // ISIZE from trailer (mod 2^32)
    uint32_t crc32;             // CRC-32 from trailer
    std::string original_name;  // empty unless FNAME was set
};

// Parses a gzip stream's header and trailer without running DEFLATE.
[[nodiscard]] GzipInfo gzip_info(std::span<const uint8_t> input);

} // namespace compression
