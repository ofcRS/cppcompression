#include "gzip.h"
#include "../deflate/deflate.h"

#include <array>
#include <stdexcept>

namespace compression {

// ── CRC-32 (ISO 3309) ─────────────────────────────────────────────────────

static constexpr auto make_crc_table() {
    std::array<uint32_t, 256> table{};
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (int j = 0; j < 8; ++j) {
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
        }
        table[i] = crc;
    }
    return table;
}

static constexpr auto kCrcTable = make_crc_table();

uint32_t crc32(std::span<const uint8_t> data) {
    uint32_t crc = 0xFFFFFFFFu;
    for (auto byte : data) {
        crc = kCrcTable[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

// ── Helpers ────────────────────────────────────────────────────────────────

static void write_le16(std::vector<uint8_t>& out, uint16_t val) {
    out.push_back(static_cast<uint8_t>(val));
    out.push_back(static_cast<uint8_t>(val >> 8));
}

static void write_le32(std::vector<uint8_t>& out, uint32_t val) {
    out.push_back(static_cast<uint8_t>(val));
    out.push_back(static_cast<uint8_t>(val >> 8));
    out.push_back(static_cast<uint8_t>(val >> 16));
    out.push_back(static_cast<uint8_t>(val >> 24));
}

static uint16_t read_le16(std::span<const uint8_t> data, std::size_t offset) {
    return static_cast<uint16_t>(data[offset])
         | (static_cast<uint16_t>(data[offset + 1]) << 8);
}

static uint32_t read_le32(std::span<const uint8_t> data, std::size_t offset) {
    return static_cast<uint32_t>(data[offset])
         | (static_cast<uint32_t>(data[offset + 1]) << 8)
         | (static_cast<uint32_t>(data[offset + 2]) << 16)
         | (static_cast<uint32_t>(data[offset + 3]) << 24);
}

// ── Gzip format (RFC 1952) ─────────────────────────────────────────────────
//
//  Header (10 bytes minimum):
//    ID1=0x1f  ID2=0x8b  CM=8  FLG  MTIME(4)  XFL  OS
//
//  Optional header fields (depending on FLG bits):
//    FEXTRA:   XLEN(2) + extra data
//    FNAME:    zero-terminated original filename
//    FCOMMENT: zero-terminated comment
//    FHCRC:    CRC16 of header
//
//  Compressed data (raw DEFLATE)
//
//  Trailer (8 bytes):
//    CRC32(4)  ISIZE(4)

// FLG bit masks
static constexpr uint8_t FTEXT    = 1 << 0;
static constexpr uint8_t FHCRC    = 1 << 1;
static constexpr uint8_t FEXTRA   = 1 << 2;
static constexpr uint8_t FNAME    = 1 << 3;
static constexpr uint8_t FCOMMENT = 1 << 4;

std::vector<uint8_t> gzip_compress(std::span<const uint8_t> input) {
    std::vector<uint8_t> output;

    // ── Header (10 bytes) ──────────────────────────────────────────────
    output.push_back(0x1F);   // ID1
    output.push_back(0x8B);   // ID2
    output.push_back(8);      // CM = deflate
    output.push_back(0);      // FLG = no optional fields
    write_le32(output, 0);    // MTIME = 0 (not available)
    output.push_back(0);      // XFL = 0
    output.push_back(0xFF);   // OS = unknown

    // ── Compressed data ────────────────────────────────────────────────
    auto compressed = deflate_compress(input);
    output.insert(output.end(), compressed.begin(), compressed.end());

    // ── Trailer (8 bytes) ──────────────────────────────────────────────
    write_le32(output, crc32(input));
    write_le32(output, static_cast<uint32_t>(input.size())); // ISIZE mod 2^32

    return output;
}

std::vector<uint8_t> gzip_decompress(std::span<const uint8_t> input) {
    if (input.size() < 18) {
        throw std::runtime_error("gzip: input too short");
    }

    // ── Validate header ────────────────────────────────────────────────
    if (input[0] != 0x1F || input[1] != 0x8B) {
        throw std::runtime_error("gzip: invalid magic bytes");
    }
    if (input[2] != 8) {
        throw std::runtime_error("gzip: unsupported compression method");
    }

    uint8_t flg = input[3];
    // bytes 4-7: MTIME (ignored)
    // byte 8: XFL (ignored)
    // byte 9: OS (ignored)

    std::size_t pos = 10;

    // Skip optional header fields
    if (flg & FEXTRA) {
        if (pos + 2 > input.size()) throw std::runtime_error("gzip: truncated FEXTRA");
        uint16_t xlen = read_le16(input, pos);
        pos += 2 + xlen;
    }
    if (flg & FNAME) {
        while (pos < input.size() && input[pos] != 0) ++pos;
        ++pos; // skip null terminator
    }
    if (flg & FCOMMENT) {
        while (pos < input.size() && input[pos] != 0) ++pos;
        ++pos;
    }
    if (flg & FHCRC) {
        pos += 2; // skip header CRC16
    }

    if (pos + 8 > input.size()) {
        throw std::runtime_error("gzip: truncated data");
    }

    // ── Decompress DEFLATE payload ─────────────────────────────────────
    auto deflate_data = input.subspan(pos, input.size() - pos - 8);
    auto decompressed = deflate_decompress(deflate_data);

    // ── Verify trailer ─────────────────────────────────────────────────
    std::size_t trailer_pos = input.size() - 8;
    uint32_t expected_crc = read_le32(input, trailer_pos);
    uint32_t expected_size = read_le32(input, trailer_pos + 4);

    uint32_t actual_crc = crc32(std::span<const uint8_t>(decompressed));
    uint32_t actual_size = static_cast<uint32_t>(decompressed.size());

    if (actual_crc != expected_crc) {
        throw std::runtime_error("gzip: CRC32 mismatch");
    }
    if (actual_size != expected_size) {
        throw std::runtime_error("gzip: size mismatch");
    }

    return decompressed;
}

GzipInfo gzip_info(std::span<const uint8_t> input) {
    if (input.size() < 18) {
        throw std::runtime_error("gzip: input too short");
    }
    if (input[0] != 0x1F || input[1] != 0x8B) {
        throw std::runtime_error("gzip: invalid magic bytes");
    }
    if (input[2] != 8) {
        throw std::runtime_error("gzip: unsupported compression method");
    }

    GzipInfo info{};
    uint8_t flg = input[3];
    info.mtime = read_le32(input, 4);

    std::size_t pos = 10;

    if (flg & FEXTRA) {
        if (pos + 2 > input.size()) throw std::runtime_error("gzip: truncated FEXTRA");
        uint16_t xlen = read_le16(input, pos);
        pos += 2 + xlen;
    }
    if (flg & FNAME) {
        std::size_t start = pos;
        while (pos < input.size() && input[pos] != 0) ++pos;
        if (pos >= input.size()) throw std::runtime_error("gzip: truncated FNAME");
        info.original_name.assign(
            reinterpret_cast<const char*>(input.data() + start), pos - start);
        ++pos; // skip null terminator
    }
    if (flg & FCOMMENT) {
        while (pos < input.size() && input[pos] != 0) ++pos;
        if (pos >= input.size()) throw std::runtime_error("gzip: truncated FCOMMENT");
        ++pos;
    }
    if (flg & FHCRC) {
        pos += 2;
    }

    if (pos + 8 > input.size()) {
        throw std::runtime_error("gzip: truncated data");
    }

    std::size_t trailer_pos = input.size() - 8;
    info.crc32 = read_le32(input, trailer_pos);
    info.uncompressed_size = read_le32(input, trailer_pos + 4);
    return info;
}

} // namespace compression
