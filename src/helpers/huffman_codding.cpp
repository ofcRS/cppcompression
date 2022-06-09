#include "iostream"
#include "string"
#include "vector"
#include "unordered_map"
#include "cmath"

struct NodeTree {
    float value;
    NodeTree *left;
    NodeTree *right;
};

std::vector<NodeTree>::iterator get_smallest_node(std::vector<NodeTree> &list) {
    std::vector<NodeTree>::iterator min = list.begin();
    for (std::vector<NodeTree>::iterator it = list.begin(); it != list.end(); it++) {
        if (it->value < min->value) {
            min = it;
        }
    }

    return min;
};

void print_vector(std::vector<NodeTree> list) {
    for (auto el : list) {
        std::cout << el.value << " ";
    }
    std::cout << std::endl;
}

void print_tree(NodeTree head, int number_of_items) {
    int vertical_size = std::log2(number_of_items);

    std::vector<NodeTree> queue;
    queue.push_back(head);

    while (queue.size() > 0) {
        NodeTree current_element = *queue.begin();

        if (current_element.left != nullptr) {
            std::cout << "Left: " << current_element.left->value << std::endl;
            queue.push_back(*current_element.left);
        }
        if (current_element.right != nullptr) {
            std::cout << "Right: " << current_element.right->value << std::endl;
            queue.push_back(*current_element.right);
        }
        queue.erase(queue.begin());
    }
}

int main() {
    std::string str = "abcdefff";

    std::unordered_map<char, float> chars_counting;

    for (int i = 0; i < str.length(); i++) {
        char c = str[i];
        chars_counting[c] += 1;
    }

    for (const auto &n: chars_counting) {
        chars_counting[n.first] = chars_counting[n.first] / str.length();
    }

    std::vector<NodeTree> nodes_list;


    for (const auto &n: chars_counting) {
        NodeTree node = NodeTree();
        node.left = nullptr;
        node.right = nullptr;
        node.value = n.second;
        nodes_list.push_back(node);
    }


    while (nodes_list.size() > 1) {
        NodeTree node = NodeTree();

        float sum = 0;

        for (int i = 0; i < 2; i++) {
            auto smallest_node_it = get_smallest_node(nodes_list);
            NodeTree* copy_of_smallest_node = new NodeTree;
            copy_of_smallest_node->value = smallest_node_it->value;
            copy_of_smallest_node->left = smallest_node_it->left;
            copy_of_smallest_node->right = smallest_node_it->right;

            if (i == 0) {
                node.left = copy_of_smallest_node;
            } else {
                node.right = copy_of_smallest_node;
            }
            sum += copy_of_smallest_node->value;
            nodes_list.erase(smallest_node_it);
        }

        node.value = sum;

        nodes_list.push_back(node);

//        print_vector(nodes_list);
    }
    print_tree(nodes_list[0], 1);
}
