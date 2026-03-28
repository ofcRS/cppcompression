#include <cassert>
#include <format>
#include <iostream>
#include <string_view>

#include "lz77/lz77.h"

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
    auto tokens = compression::lz77_compress(test_input);

    for (const auto& [offset, length, next_char] : tokens) {
        std::cout << std::format("({},{},{})", offset, length, next_char) << '\n';
    }

    auto decompressed = compression::lz77_decompress(tokens);
    std::cout << '\n' << decompressed << '\n';

    assert(decompressed == test_input);
    std::cout << "\nRound-trip OK: decompressed matches original\n";

    return 0;
}
