#include "lz77.h"

#include <algorithm>

namespace compression {

std::vector<Token> lz77_compress(std::string_view input, std::size_t window_size) {
    std::vector<Token> output;
    std::size_t pos = 0;

    while (pos < input.size()) {
        std::size_t best_length = 0;
        std::size_t best_offset = 0;

        std::size_t search_start = (pos > window_size) ? pos - window_size : 0;

        for (std::size_t i = search_start; i < pos; ++i) {
            std::size_t match_len = 0;
            while (pos + match_len < input.size()
                   && i + match_len < pos
                   && input[pos + match_len] == input[i + match_len]) {
                ++match_len;
            }

            if (match_len > best_length) {
                best_length = match_len;
                best_offset = pos - i;
            }
        }

        if (best_length > 0 && pos + best_length < input.size()) {
            output.emplace_back(Token{
                .offset = best_offset,
                .length = best_length,
                .next_char = input[pos + best_length],
            });
            pos += best_length + 1;
        } else if (best_length > 0 && pos + best_length == input.size()) {
            // Match extends to the very end — emit without next_char
            output.emplace_back(Token{
                .offset = best_offset,
                .length = best_length,
                .next_char = '\0',
            });
            pos += best_length;
        } else {
            output.emplace_back(Token{
                .offset = 0,
                .length = 0,
                .next_char = input[pos],
            });
            ++pos;
        }
    }

    return output;
}

std::string lz77_decompress(std::span<const Token> tokens) {
    std::string result;
    result.reserve(tokens.size() * 2);

    for (const auto& token : tokens) {
        if (token.offset > 0) {
            std::size_t start = result.size() - token.offset;
            for (std::size_t i = 0; i < token.length; ++i) {
                result += result[start + i];
            }
        }
        if (token.next_char != '\0') {
            result += token.next_char;
        }
    }

    return result;
}

} // namespace compression
