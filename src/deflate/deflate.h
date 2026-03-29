#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace compression {

[[nodiscard]] std::vector<uint8_t> deflate_compress(std::span<const uint8_t> input);
[[nodiscard]] std::vector<uint8_t> deflate_decompress(std::span<const uint8_t> input);

} // namespace compression
