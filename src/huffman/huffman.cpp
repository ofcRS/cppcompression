#include "huffman.h"

#include <algorithm>
#include <numeric>
#include <queue>
#include <utility>

namespace compression {

// Internal tree node for building Huffman tree from frequencies
namespace {

struct BuildNode {
    uint32_t freq;
    int symbol;  // -1 for internal nodes
    int left;    // index into pool
    int right;
};

} // namespace

std::vector<uint8_t> huffman_lengths_from_frequencies(
    std::span<const uint32_t> frequencies, uint8_t max_length) {

    int n = static_cast<int>(frequencies.size());
    std::vector<uint8_t> lengths(n, 0);

    // Count non-zero frequencies
    std::vector<int> symbols;
    for (int i = 0; i < n; ++i) {
        if (frequencies[i] > 0) {
            symbols.push_back(i);
        }
    }

    if (symbols.empty()) return lengths;

    if (symbols.size() == 1) {
        lengths[symbols[0]] = 1;
        return lengths;
    }

    // Build Huffman tree using a priority queue
    // Pool of nodes: first n entries are leaves, rest are internal
    std::vector<BuildNode> pool;
    pool.reserve(2 * symbols.size());

    auto cmp = [&](int a, int b) { return pool[a].freq > pool[b].freq; };
    std::priority_queue<int, std::vector<int>, decltype(cmp)> pq(cmp);

    for (int sym : symbols) {
        int idx = static_cast<int>(pool.size());
        pool.push_back({frequencies[sym], sym, -1, -1});
        pq.push(idx);
    }

    while (pq.size() > 1) {
        int left = pq.top(); pq.pop();
        int right = pq.top(); pq.pop();
        int idx = static_cast<int>(pool.size());
        pool.push_back({pool[left].freq + pool[right].freq, -1, left, right});
        pq.push(idx);
    }

    // Compute depths (code lengths) via DFS
    int root = pq.top();
    struct StackEntry { int node; int depth; };
    std::vector<StackEntry> stack;
    stack.push_back({root, 0});

    while (!stack.empty()) {
        auto [node, depth] = stack.back();
        stack.pop_back();

        if (pool[node].symbol >= 0) {
            lengths[pool[node].symbol] = static_cast<uint8_t>(depth);
        } else {
            if (pool[node].left >= 0) stack.push_back({pool[node].left, depth + 1});
            if (pool[node].right >= 0) stack.push_back({pool[node].right, depth + 1});
        }
    }

    // Enforce max_length by redistributing (package-merge simplification)
    // For DEFLATE, max is 15 for lit/len and distance, 7 for code length alphabet
    bool needs_fix = false;
    for (int sym : symbols) {
        if (lengths[sym] > max_length) {
            needs_fix = true;
            break;
        }
    }

    if (needs_fix) {
        // Simple length limiting: cap at max_length and redistribute
        // Sort symbols by length descending
        std::sort(symbols.begin(), symbols.end(), [&](int a, int b) {
            return lengths[a] > lengths[b];
        });

        for (int sym : symbols) {
            if (lengths[sym] > max_length) {
                lengths[sym] = max_length;
            }
        }

        // Kraft inequality check and fix
        // Sum of 2^(-length) must equal 1
        // This is a simplified approach; full package-merge is more optimal
        uint32_t kraft = 0;
        for (int sym : symbols) {
            kraft += 1u << (max_length - lengths[sym]);
        }
        uint32_t target = 1u << max_length;

        while (kraft > target) {
            // Find the longest code and shorten it
            for (int i = static_cast<int>(symbols.size()) - 1; i >= 0; --i) {
                if (lengths[symbols[i]] < max_length) {
                    // Increase this length by 1 to free up space
                    kraft -= 1u << (max_length - lengths[symbols[i]]);
                    lengths[symbols[i]]++;
                    kraft += 1u << (max_length - lengths[symbols[i]]);
                    if (kraft <= target) break;
                }
            }
        }
    }

    return lengths;
}

std::vector<HuffmanCode> huffman_codes_from_lengths(std::span<const uint8_t> lengths) {
    int n = static_cast<int>(lengths.size());
    std::vector<HuffmanCode> codes(n);

    // Find max length
    uint8_t max_len = 0;
    for (auto len : lengths) {
        if (len > max_len) max_len = len;
    }
    if (max_len == 0) return codes;

    // Count number of codes at each length
    std::vector<int> bl_count(max_len + 1, 0);
    for (auto len : lengths) {
        if (len > 0) bl_count[len]++;
    }

    // Compute starting code for each length (RFC 1951 algorithm)
    std::vector<uint16_t> next_code(max_len + 1, 0);
    uint16_t code = 0;
    for (int bits = 1; bits <= max_len; ++bits) {
        code = (code + bl_count[bits - 1]) << 1;
        next_code[bits] = code;
    }

    // Assign codes to symbols
    for (int i = 0; i < n; ++i) {
        if (lengths[i] > 0) {
            codes[i].code = next_code[lengths[i]]++;
            codes[i].length = lengths[i];
        }
    }

    return codes;
}

std::vector<HuffmanNode> huffman_build_decode_tree(std::span<const uint8_t> lengths) {
    auto codes = huffman_codes_from_lengths(lengths);

    std::vector<HuffmanNode> tree;
    tree.push_back({}); // root node at index 0

    for (int sym = 0; sym < static_cast<int>(codes.size()); ++sym) {
        if (codes[sym].length == 0) continue;

        int node = 0;
        for (int bit = codes[sym].length - 1; bit >= 0; --bit) {
            int b = (codes[sym].code >> bit) & 1;
            if (tree[node].children[b] < 0) {
                tree[node].children[b] = static_cast<int>(tree.size());
                tree.push_back({});
            }
            node = tree[node].children[b];
        }
        tree[node].symbol = sym;
    }

    return tree;
}

} // namespace compression
