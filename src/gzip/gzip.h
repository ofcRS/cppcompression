#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace compression {

// Computes CRC-32 checksum (ISO 3309 / ITU-T V.42 polynomial).
[[nodiscard]] uint32_t crc32(std::span<const uint8_t> data);

// Compresses data into gzip format (RFC 1952).
// The output is compatible with standard gunzip / zlib.
[[nodiscard]] std::vector<uint8_t> gzip_compress(std::span<const uint8_t> input);

// Decompresses gzip-formatted data (RFC 1952).
[[nodiscard]] std::vector<uint8_t> gzip_decompress(std::span<const uint8_t> input);

} // namespace compression
