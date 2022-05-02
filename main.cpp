#include <iostream>
#include "string"
#include "vector"

const std::string str = "sir sid eastman Nulla lobortis nunc at cursus malesuada. Integer quis tortor diam. In et congue tortor. Sed a est sed nisi tincidunt vulputate a eu urna. Nulla felis lorem, euismod vel erat id, scelerisque faucibus velit. Curabitur hendrerit leo eu nunc ornare bibendum. Nulla vulputate mollis ligula, ornare venenatis libero aliquet sed. Suspendisse et ultrices lectus. Vestibulum facilisis mattis sem a aliquet. Etiam at sodales est. Sed non consequat est. Nam vestibulum, risus ut rhoncus eleifend, ligula nisi malesuada ipsum, quis mattis odio sem sit amet lorem. Sed tempor accumsan justo, ut rutrum felis consectetur non. Etiam ultricies mi mauris, sed pretium ipsum tempus sed. Aenean commodo facilisis sem.";

const int BUFFER_LENGTH = 10;

struct output_token {
    int offset;
    int length;
    char next_symbol;
};

int main() {
    std::vector<output_token> output;

    for (char i : str) {
        output_token token = output_token();
        token.length = 0;
        token.offset = 0;
        token.next_symbol = i;
    }

    for (auto &item : output) {
        std::cout << "(" << item.offset << "," << item.length << "," << item.next_symbol << ")" << std::endl;
    }

    return 0;
}
