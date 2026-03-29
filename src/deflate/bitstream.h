#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace compression {

// Writes bits LSB-first into a byte buffer (DEFLATE bit ordering per RFC 1951)
class BitWriter {
public:
    void write_bits(uint32_t value, int count);
    void write_byte(uint8_t byte);
    void flush();

    [[nodiscard]] std::span<const uint8_t> data() const { return buffer_; }
    [[nodiscard]] std::size_t bit_count() const { return buffer_.size() * 8 - (8 - bits_in_current_) % 8; }

private:
    std::vector<uint8_t> buffer_;
    uint8_t current_byte_ = 0;
    int bits_in_current_ = 0;
};

// Reads bits LSB-first from a byte buffer
class BitReader {
public:
    explicit BitReader(std::span<const uint8_t> data) : data_(data) {}

    [[nodiscard]] uint32_t read_bits(int count);
    [[nodiscard]] uint8_t read_byte();
    void align_to_byte();

    [[nodiscard]] bool eof() const { return byte_pos_ >= data_.size(); }
    [[nodiscard]] std::size_t bit_position() const { return byte_pos_ * 8 + bit_pos_; }

private:
    std::span<const uint8_t> data_;
    std::size_t byte_pos_ = 0;
    int bit_pos_ = 0;
};

} // namespace compression
