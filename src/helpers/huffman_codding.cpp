#include "iostream"
#include "string"
#include "vector"
#include "unordered_map"


struct NodeTree {
    int value;
    NodeTree *left;
    NodeTree *right;
};

int main() {
    std::string str = "abcdefff";
    
    std::unordered_map<char, int> chars_counting = {};

    for (int i = 0; i < str.length(); i++) {
        char c = str[i];
        chars_counting[c] += 1;
    }

    for (const auto &n : chars_counting) {
        std::cout << n.first << " " << n.second << std::endl;
    }
}
