#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace compression {

struct HuffmanCode {
    uint16_t code = 0;
    uint8_t length = 0;
};

// Builds canonical Huffman code lengths from symbol frequencies.
// Returns a vector where result[symbol] = bit length (0 means symbol not used).
[[nodiscard]] std::vector<uint8_t> huffman_lengths_from_frequencies(
    std::span<const uint32_t> frequencies, uint8_t max_length = 15);

// Builds canonical Huffman codes from code lengths.
// Returns a vector where result[symbol] = {code, length}.
[[nodiscard]] std::vector<HuffmanCode> huffman_codes_from_lengths(
    std::span<const uint8_t> lengths);

// Huffman decode tree node for bit-by-bit decoding
struct HuffmanNode {
    int children[2] = {-1, -1}; // indices into node array, -1 = no child
    int symbol = -1;            // >= 0 if this is a leaf
};

// Builds a decode tree from code lengths. Returns the node array (root at index 0).
[[nodiscard]] std::vector<HuffmanNode> huffman_build_decode_tree(
    std::span<const uint8_t> lengths);

} // namespace compression
