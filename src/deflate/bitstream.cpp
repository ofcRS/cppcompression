#include "bitstream.h"

#include <stdexcept>

namespace compression {

void BitWriter::write_bits(uint32_t value, int count) {
    for (int i = 0; i < count; ++i) {
        current_byte_ |= static_cast<uint8_t>(((value >> i) & 1) << bits_in_current_);
        ++bits_in_current_;
        if (bits_in_current_ == 8) {
            buffer_.push_back(current_byte_);
            current_byte_ = 0;
            bits_in_current_ = 0;
        }
    }
}

void BitWriter::write_byte(uint8_t byte) {
    if (bits_in_current_ == 0) {
        buffer_.push_back(byte);
    } else {
        write_bits(byte, 8);
    }
}

void BitWriter::flush() {
    if (bits_in_current_ > 0) {
        buffer_.push_back(current_byte_);
        current_byte_ = 0;
        bits_in_current_ = 0;
    }
}

uint32_t BitReader::read_bits(int count) {
    uint32_t result = 0;
    for (int i = 0; i < count; ++i) {
        if (byte_pos_ >= data_.size()) {
            throw std::runtime_error("BitReader: unexpected end of data");
        }
        uint32_t bit = (data_[byte_pos_] >> bit_pos_) & 1;
        result |= bit << i;
        ++bit_pos_;
        if (bit_pos_ == 8) {
            bit_pos_ = 0;
            ++byte_pos_;
        }
    }
    return result;
}

uint8_t BitReader::read_byte() {
    align_to_byte();
    if (byte_pos_ >= data_.size()) {
        throw std::runtime_error("BitReader: unexpected end of data");
    }
    return data_[byte_pos_++];
}

void BitReader::align_to_byte() {
    if (bit_pos_ > 0) {
        bit_pos_ = 0;
        ++byte_pos_;
    }
}

} // namespace compression
