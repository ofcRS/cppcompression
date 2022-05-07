#include "iostream"
#include "string"
#include "lz77/lz77.h"

const std::string str = "sir sid eastman Nulla lobortis nunc at cursus malesuada. Integer quis tortor diam. In et congue tortor. Sed a est sed nisi tincidunt vulputate a eu urna. Nulla felis lorem, euismod vel erat id, scelerisque faucibus velit. Curabitur hendrerit leo eu nunc ornare bibendum. Nulla vulputate mollis ligula, ornare venenatis libero aliquet sed. Suspendisse et ultrices lectus. Vestibulum facilisis mattis sem a aliquet. Etiam at sodales est. Sed non consequat est. Nam vestibulum, risus ut rhoncus eleifend, ligula nisi malesuada ipsum, quis mattis odio sem sit amet lorem. Sed tempor accumsan justo, ut rutrum felis consectetur non. Etiam ultricies mi mauris, sed pretium ipsum tempus sed. Aenean commodo facilisis sem.";
//const std::string str = "sir sid eastman";

int main() {
    auto output = LZ77().lm77_compress(str);

    for (auto &item: output) {
        std::cout << "(" << item.offset << "," << item.length << "," << item.next_symbol << ")" << std::endl;
    }

    return 0;
}
