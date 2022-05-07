#ifndef lz77
#define lz77

#include "vector"
#include "string"

struct output_token {
    int offset;
    int length;
    char next_symbol;
};

class LZ77 {
public:
    std::vector<output_token> lm77_compress(std::string str);
};

#endif
