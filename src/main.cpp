#include <cassert>
#include <format>
#include <iostream>
#include <string_view>

#include "lz77/lz77.h"
#include "deflate/deflate.h"

constexpr std::string_view test_input =
    "sir sid eastman Nulla lobortis nunc at cursus malesuada. Integer quis "
    "tortor diam. In et congue tortor. Sed a est sed nisi tincidunt vulputate "
    "a eu urna. Nulla felis lorem, euismod vel erat id, scelerisque faucibus "
    "velit. Curabitur hendrerit leo eu nunc ornare bibendum. Nulla vulputate "
    "mollis ligula, ornare venenatis libero aliquet sed. Suspendisse et "
    "ultrices lectus. Vestibulum facilisis mattis sem a aliquet. Etiam at "
    "sodales est. Sed non consequat est. Nam vestibulum, risus ut rhoncus "
    "eleifend, ligula nisi malesuada ipsum, quis mattis odio sem sit amet "
    "lorem. Sed tempor accumsan justo, ut rutrum felis consectetur non. Etiam "
    "ultricies mi mauris, sed pretium ipsum tempus sed. Aenean commodo "
    "facilisis sem.";

int main() {
    // ── LZ77 demo ───────────────────────────────────────────────────────
    std::cout << "=== LZ77 ===\n";
    auto tokens = compression::lz77_compress(test_input);

    for (const auto& [offset, length, next_char] : tokens) {
        std::cout << std::format("({},{},{})", offset, length, next_char);
    }
    std::cout << '\n';

    auto lz77_decompressed = compression::lz77_decompress(tokens);
    assert(lz77_decompressed == test_input);
    std::cout << "LZ77 round-trip OK (" << tokens.size() << " tokens)\n\n";

    // ── DEFLATE demo ────────────────────────────────────────────────────
    std::cout << "=== DEFLATE ===\n";
    auto input_bytes = std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(test_input.data()), test_input.size());

    auto compressed = compression::deflate_compress(input_bytes);
    auto decompressed = compression::deflate_decompress(compressed);

    std::string_view decompressed_str(
        reinterpret_cast<const char*>(decompressed.data()), decompressed.size());

    assert(decompressed_str == test_input);

    std::cout << std::format("Original:     {} bytes\n", test_input.size());
    std::cout << std::format("Compressed:   {} bytes\n", compressed.size());
    std::cout << std::format("Ratio:        {:.1f}%\n",
        100.0 * static_cast<double>(compressed.size()) / static_cast<double>(test_input.size()));
    std::cout << "DEFLATE round-trip OK\n";

    return 0;
}
