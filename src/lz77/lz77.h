#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace compression {

struct Token {
    std::size_t offset = 0;
    std::size_t length = 0;
    char next_char = '\0';
};

[[nodiscard]] std::vector<Token> lz77_compress(std::string_view input,
                                               std::size_t window_size = 4096);

[[nodiscard]] std::string lz77_decompress(std::span<const Token> tokens);

} // namespace compression
