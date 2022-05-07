#include <iostream>
#include "string"
#include "vector"

//const std::string str = "sir sid eastman Nulla lobortis nunc at cursus malesuada. Integer quis tortor diam. In et congue tortor. Sed a est sed nisi tincidunt vulputate a eu urna. Nulla felis lorem, euismod vel erat id, scelerisque faucibus velit. Curabitur hendrerit leo eu nunc ornare bibendum. Nulla vulputate mollis ligula, ornare venenatis libero aliquet sed. Suspendisse et ultrices lectus. Vestibulum facilisis mattis sem a aliquet. Etiam at sodales est. Sed non consequat est. Nam vestibulum, risus ut rhoncus eleifend, ligula nisi malesuada ipsum, quis mattis odio sem sit amet lorem. Sed tempor accumsan justo, ut rutrum felis consectetur non. Etiam ultricies mi mauris, sed pretium ipsum tempus sed. Aenean commodo facilisis sem.";
const std::string str = "sir sid eastman";


const int BUFFER_LENGTH = 10;

struct output_token {
    int offset;
    int length;
    char next_symbol;
};

int main() {
    std::vector<output_token> output;

    int buffer_start = 0;

    while (buffer_start < str.length()) {
        std::cout << "buffer_start = " << buffer_start << std::endl;  
        int max_match_length = 0;
	int max_match_offset = 0;
        for (int i = buffer_start - 1; i >= 0; i--) {
               int current_length = 0;

               if (str[buffer_start] == str[i]) {
                       int j = 0;
                       while (str[buffer_start + j] == str[i + j]) {
                               current_length++;
                               j++;
                       }
               }
	       if (current_length > max_match_length) {
		       max_match_length = current_length;
		       max_match_offset = buffer_start - i;
	       }
               //std::cout << "max match offset = " << max_match_offset << std::endl;

        }
        if (max_match_length > 0) {
               auto token = output_token();
               token.offset = max_match_offset;
               token.length = max_match_length;
               token.next_symbol = str[buffer_start + max_match_length];
	       output.push_back(token);
	       buffer_start += max_match_length + 1;
	} else {
               auto token = output_token();
               token.offset = 0;
               token.length = 0;
               token.next_symbol = str[buffer_start];
	       output.push_back(token);
               buffer_start++;
	}
    }

    for (auto &item : output) {
        std::cout << "(" << item.offset << "," << item.length << "," << item.next_symbol << ")" << std::endl;
    }

    std::cout << "Hello there!" << std::endl;
    return 0;
}
