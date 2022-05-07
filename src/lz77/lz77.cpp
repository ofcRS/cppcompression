#include <iostream>
#include "string"
#include "vector"

#include "lz77.h"

std::vector<output_token> LZ77::lm77_compress(std::string str) {
    std::vector<output_token> output;

    int buffer_start = 0;

    while (buffer_start < str.length()) {
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

    return output;
}