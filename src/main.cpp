#include <cassert>
#include <cstdio>
#include <format>
#include <iostream>
#include <string_view>

#include "lz77/lz77.h"
#include "deflate/deflate.h"
#include "gzip/gzip.h"

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
    std::cout << "DEFLATE round-trip OK\n\n";

    // ── GZIP demo ──────────────────────────────────────────────────────
    std::cout << "=== GZIP ===\n";
    auto gz_compressed = compression::gzip_compress(input_bytes);
    auto gz_decompressed = compression::gzip_decompress(gz_compressed);

    std::string_view gz_decompressed_str(
        reinterpret_cast<const char*>(gz_decompressed.data()), gz_decompressed.size());

    assert(gz_decompressed_str == test_input);

    std::cout << std::format("Original:     {} bytes\n", test_input.size());
    std::cout << std::format("Gzip:         {} bytes (DEFLATE + 18 bytes header/trailer)\n", gz_compressed.size());
    std::cout << std::format("Ratio:        {:.1f}%\n",
        100.0 * static_cast<double>(gz_compressed.size()) / static_cast<double>(test_input.size()));
    std::cout << "GZIP round-trip OK\n";

    // Write gzip output to file for verification with system gunzip
    if (auto* f = std::fopen("test_output.gz", "wb")) {
        std::fwrite(gz_compressed.data(), 1, gz_compressed.size(), f);
        std::fclose(f);
        std::cout << "Written test_output.gz (verify with: gunzip -k test_output.gz)\n";
    }

    return 0;
}
