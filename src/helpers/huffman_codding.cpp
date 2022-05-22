#include "iostream"
#include "string"
#include "vector"
#include "unordered_map"


struct NodeTree {
    float value;
    NodeTree *left;
    NodeTree *right;
};

int main() {
    std::string str = "abcdefff";
    
    std::unordered_map<char, float> chars_counting = {};

    for (int i = 0; i < str.length(); i++) {
        char c = str[i];
        chars_counting[c] += 1;
    }

    for (const auto &n : chars_counting) {
        chars_counting[n.first] = chars_counting[n.first] / str.length(); 
    }
    
    std::vector<NodeTree> nodes_list;
    

    for (const auto &n : chars_counting) {
        NodeTree node = NodeTree();
        node.left = nullptr;
        node.right = nullptr;
        node.value = n.second;
        nodes_list.push_back(node);
    }

    while (nodes_list.length() > 0) {
        vector<int> min_indecies;

        while (min_indecies.length() < 2) {
            int min_value = INT_MAX;
            
            for (int i = 0; i < nodes_list.length(); i++) {
                
            }
        }
    }

    for (const auto &item : nodes_list) {
        std::cout << item.value << std::endl;
    }
}
