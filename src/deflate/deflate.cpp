#include "deflate.h"
#include "bitstream.h"
#include "../huffman/huffman.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string_view>
#include <variant>

namespace compression {

// ── DEFLATE tables (RFC 1951) ───────────────────────────────────────────────

struct CodeEntry { uint16_t base; uint8_t extra_bits; };

// Length codes 257..285 → base length + extra bits
static constexpr std::array<CodeEntry, 29> kLengthTable = {{
    {3,0},{4,0},{5,0},{6,0},{7,0},{8,0},{9,0},{10,0},
    {11,1},{13,1},{15,1},{17,1},
    {19,2},{23,2},{27,2},{31,2},
    {35,3},{43,3},{51,3},{59,3},
    {67,4},{83,4},{99,4},{115,4},
    {131,5},{163,5},{195,5},{227,5},
    {258,0}
}};

// Distance codes 0..29 → base distance + extra bits
static constexpr std::array<CodeEntry, 30> kDistanceTable = {{
    {1,0},{2,0},{3,0},{4,0},
    {5,1},{7,1},
    {9,2},{13,2},
    {17,3},{25,3},
    {33,4},{49,4},
    {65,5},{97,5},
    {129,6},{193,6},
    {257,7},{385,7},
    {513,8},{769,8},
    {1025,9},{1537,9},
    {2049,10},{3073,10},
    {4097,11},{6145,11},
    {8193,12},{12289,12},
    {16385,13},{24577,13}
}};

// Code length alphabet order for dynamic Huffman
static constexpr std::array<int, 19> kCodeLengthOrder = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

// ── LZ77 for DEFLATE ────────────────────────────────────────────────────────

struct LengthDistance {
    uint16_t length;
    uint16_t distance;
};

using DeflateSymbol = std::variant<uint8_t, LengthDistance>;

static constexpr std::size_t kWindowSize = 32768;
static constexpr int kMinMatch = 3;
static constexpr int kMaxMatch = 258;

static std::vector<DeflateSymbol> deflate_lz77(std::span<const uint8_t> input) {
    std::vector<DeflateSymbol> symbols;
    std::size_t pos = 0;

    while (pos < input.size()) {
        std::size_t best_len = 0;
        std::size_t best_dist = 0;

        std::size_t search_start = (pos > kWindowSize) ? pos - kWindowSize : 0;

        for (std::size_t i = search_start; i < pos; ++i) {
            std::size_t len = 0;
            while (pos + len < input.size()
                   && len < kMaxMatch
                   && input[i + len] == input[pos + len]) {
                ++len;
            }
            if (len > best_len) {
                best_len = len;
                best_dist = pos - i;
            }
        }

        if (best_len >= kMinMatch) {
            symbols.emplace_back(LengthDistance{
                static_cast<uint16_t>(best_len),
                static_cast<uint16_t>(best_dist)
            });
            pos += best_len;
        } else {
            symbols.emplace_back(input[pos]);
            ++pos;
        }
    }

    return symbols;
}

// ── Code lookup helpers ─────────────────────────────────────────────────────

static int length_to_code(uint16_t length) {
    for (int i = 0; i < 29; ++i) {
        uint16_t lo = kLengthTable[i].base;
        uint16_t hi = lo + (1 << kLengthTable[i].extra_bits) - 1;
        if (length >= lo && length <= hi) return 257 + i;
    }
    return 285;
}

static int distance_to_code(uint16_t distance) {
    for (int i = 0; i < 30; ++i) {
        uint16_t lo = kDistanceTable[i].base;
        uint16_t hi = lo + (1 << kDistanceTable[i].extra_bits) - 1;
        if (distance >= lo && distance <= hi) return i;
    }
    return 29;
}

// Write a Huffman code to the bitstream (MSB-first code written LSB-first)
static void write_huffman_code(BitWriter& writer, const HuffmanCode& hc) {
    // Huffman codes in DEFLATE are written MSB-first (reversed from normal LSB-first)
    for (int i = hc.length - 1; i >= 0; --i) {
        writer.write_bits((hc.code >> i) & 1, 1);
    }
}

// Read one symbol from the bitstream using a decode tree
static int read_huffman_symbol(BitReader& reader, const std::vector<HuffmanNode>& tree) {
    int node = 0;
    while (tree[node].symbol < 0) {
        int bit = static_cast<int>(reader.read_bits(1));
        int next = tree[node].children[bit];
        if (next < 0) {
            throw std::runtime_error("deflate: invalid Huffman code");
        }
        node = next;
    }
    return tree[node].symbol;
}

// ── Fixed Huffman tables ────────────────────────────────────────────────────

static std::vector<uint8_t> fixed_litlen_lengths() {
    std::vector<uint8_t> lengths(288);
    for (int i = 0; i <= 143; ++i) lengths[i] = 8;
    for (int i = 144; i <= 255; ++i) lengths[i] = 9;
    for (int i = 256; i <= 279; ++i) lengths[i] = 7;
    for (int i = 280; i <= 287; ++i) lengths[i] = 8;
    return lengths;
}

static std::vector<uint8_t> fixed_distance_lengths() {
    return std::vector<uint8_t>(32, 5);
}

// ── Dynamic Huffman encode/decode ───────────────────────────────────────────

static void write_dynamic_trees(BitWriter& writer,
                                std::span<const uint8_t> litlen_lengths,
                                std::span<const uint8_t> dist_lengths) {
    // Determine HLIT and HDIST
    int hlit = 286; // at least 257
    while (hlit > 257 && litlen_lengths[hlit - 1] == 0) --hlit;
    int hdist = 30;  // at least 1
    while (hdist > 1 && dist_lengths[hdist - 1] == 0) --hdist;

    // Combine litlen and distance lengths for RLE encoding
    std::vector<uint8_t> all_lengths;
    all_lengths.insert(all_lengths.end(), litlen_lengths.begin(), litlen_lengths.begin() + hlit);
    all_lengths.insert(all_lengths.end(), dist_lengths.begin(), dist_lengths.begin() + hdist);

    // RLE encode the combined lengths
    struct RLEEntry { int symbol; int extra; int extra_bits; };
    std::vector<RLEEntry> rle;

    for (std::size_t i = 0; i < all_lengths.size(); ) {
        uint8_t len = all_lengths[i];
        std::size_t run = 1;
        while (i + run < all_lengths.size() && all_lengths[i + run] == len) ++run;

        if (len == 0) {
            while (run > 0) {
                if (run >= 11) {
                    int repeat = std::min(run, std::size_t(138));
                    rle.push_back({18, static_cast<int>(repeat - 11), 7});
                    i += repeat;
                    run -= repeat;
                } else if (run >= 3) {
                    int repeat = std::min(run, std::size_t(10));
                    rle.push_back({17, static_cast<int>(repeat - 3), 3});
                    i += repeat;
                    run -= repeat;
                } else {
                    rle.push_back({0, 0, 0});
                    ++i;
                    --run;
                }
            }
        } else {
            rle.push_back({len, 0, 0});
            ++i;
            --run;
            while (run > 0) {
                if (run >= 3) {
                    int repeat = std::min(run, std::size_t(6));
                    rle.push_back({16, static_cast<int>(repeat - 3), 2});
                    i += repeat;
                    run -= repeat;
                } else {
                    rle.push_back({len, 0, 0});
                    ++i;
                    --run;
                }
            }
        }
    }

    // Count frequencies for the code length alphabet (0-18)
    std::vector<uint32_t> cl_freqs(19, 0);
    for (auto& entry : rle) {
        cl_freqs[entry.symbol]++;
    }

    auto cl_lengths = huffman_lengths_from_frequencies(cl_freqs, 7);
    auto cl_codes = huffman_codes_from_lengths(cl_lengths);

    // Determine HCLEN
    int hclen = 19;
    while (hclen > 4 && cl_lengths[kCodeLengthOrder[hclen - 1]] == 0) --hclen;

    // Write header
    writer.write_bits(hlit - 257, 5);
    writer.write_bits(hdist - 1, 5);
    writer.write_bits(hclen - 4, 4);

    // Write code length code lengths in the specified order
    for (int i = 0; i < hclen; ++i) {
        writer.write_bits(cl_lengths[kCodeLengthOrder[i]], 3);
    }

    // Write RLE-encoded lengths using the code length Huffman codes
    for (auto& entry : rle) {
        write_huffman_code(writer, cl_codes[entry.symbol]);
        if (entry.extra_bits > 0) {
            writer.write_bits(entry.extra, entry.extra_bits);
        }
    }
}

static void read_dynamic_trees(BitReader& reader,
                               std::vector<HuffmanNode>& litlen_tree,
                               std::vector<HuffmanNode>& dist_tree) {
    int hlit = static_cast<int>(reader.read_bits(5)) + 257;
    int hdist = static_cast<int>(reader.read_bits(5)) + 1;
    int hclen = static_cast<int>(reader.read_bits(4)) + 4;

    // Read code length code lengths
    std::vector<uint8_t> cl_lengths(19, 0);
    for (int i = 0; i < hclen; ++i) {
        cl_lengths[kCodeLengthOrder[i]] = static_cast<uint8_t>(reader.read_bits(3));
    }

    auto cl_tree = huffman_build_decode_tree(cl_lengths);

    // Decode literal/length + distance code lengths
    std::vector<uint8_t> all_lengths;
    int total = hlit + hdist;

    while (static_cast<int>(all_lengths.size()) < total) {
        int sym = read_huffman_symbol(reader, cl_tree);

        if (sym < 16) {
            all_lengths.push_back(static_cast<uint8_t>(sym));
        } else if (sym == 16) {
            int repeat = static_cast<int>(reader.read_bits(2)) + 3;
            uint8_t prev = all_lengths.empty() ? 0 : all_lengths.back();
            for (int i = 0; i < repeat; ++i) all_lengths.push_back(prev);
        } else if (sym == 17) {
            int repeat = static_cast<int>(reader.read_bits(3)) + 3;
            for (int i = 0; i < repeat; ++i) all_lengths.push_back(0);
        } else if (sym == 18) {
            int repeat = static_cast<int>(reader.read_bits(7)) + 11;
            for (int i = 0; i < repeat; ++i) all_lengths.push_back(0);
        }
    }

    std::vector<uint8_t> ll(all_lengths.begin(), all_lengths.begin() + hlit);
    std::vector<uint8_t> dl(all_lengths.begin() + hlit, all_lengths.begin() + hlit + hdist);

    litlen_tree = huffman_build_decode_tree(ll);
    dist_tree = huffman_build_decode_tree(dl);
}

// ── Compress ────────────────────────────────────────────────────────────────

std::vector<uint8_t> deflate_compress(std::span<const uint8_t> input) {
    if (input.empty()) {
        // Empty input → single empty stored block
        BitWriter writer;
        writer.write_bits(1, 1); // BFINAL
        writer.write_bits(0, 2); // BTYPE = stored
        writer.flush();
        writer.write_bits(0, 16); // LEN = 0
        writer.write_bits(0xFFFF, 16); // NLEN = ~0
        auto d = writer.data();
        return {d.begin(), d.end()};
    }

    auto symbols = deflate_lz77(input);

    // Count frequencies for literal/length and distance alphabets
    std::vector<uint32_t> litlen_freqs(286, 0);
    std::vector<uint32_t> dist_freqs(30, 0);

    for (auto& sym : symbols) {
        if (auto* lit = std::get_if<uint8_t>(&sym)) {
            litlen_freqs[*lit]++;
        } else {
            auto& ld = std::get<LengthDistance>(sym);
            litlen_freqs[length_to_code(ld.length)]++;
            dist_freqs[distance_to_code(ld.distance)]++;
        }
    }
    litlen_freqs[256] = 1; // end-of-block

    // Build dynamic Huffman trees
    auto litlen_lengths = huffman_lengths_from_frequencies(litlen_freqs);
    auto dist_lengths = huffman_lengths_from_frequencies(dist_freqs);

    // Ensure at least one distance code exists (RFC 1951 requirement)
    bool has_distance = false;
    for (auto len : dist_lengths) {
        if (len > 0) { has_distance = true; break; }
    }
    if (!has_distance) {
        dist_lengths.resize(30, 0);
        dist_lengths[0] = 1;
    }

    auto litlen_codes = huffman_codes_from_lengths(litlen_lengths);
    auto dist_codes = huffman_codes_from_lengths(dist_lengths);

    BitWriter writer;

    // Write block header: BFINAL=1, BTYPE=10 (dynamic Huffman)
    writer.write_bits(1, 1);  // BFINAL
    writer.write_bits(2, 2);  // BTYPE = dynamic

    // Write dynamic tree definitions
    write_dynamic_trees(writer, litlen_lengths, dist_lengths);

    // Encode data
    for (auto& sym : symbols) {
        if (auto* lit = std::get_if<uint8_t>(&sym)) {
            write_huffman_code(writer, litlen_codes[*lit]);
        } else {
            auto& ld = std::get<LengthDistance>(sym);

            // Length
            int lcode = length_to_code(ld.length);
            write_huffman_code(writer, litlen_codes[lcode]);
            int lidx = lcode - 257;
            if (kLengthTable[lidx].extra_bits > 0) {
                writer.write_bits(ld.length - kLengthTable[lidx].base,
                                  kLengthTable[lidx].extra_bits);
            }

            // Distance
            int dcode = distance_to_code(ld.distance);
            write_huffman_code(writer, dist_codes[dcode]);
            if (kDistanceTable[dcode].extra_bits > 0) {
                writer.write_bits(ld.distance - kDistanceTable[dcode].base,
                                  kDistanceTable[dcode].extra_bits);
            }
        }
    }

    // End-of-block
    write_huffman_code(writer, litlen_codes[256]);
    writer.flush();

    auto d = writer.data();
    return {d.begin(), d.end()};
}

// ── Decompress ──────────────────────────────────────────────────────────────

std::vector<uint8_t> deflate_decompress(std::span<const uint8_t> input) {
    BitReader reader(input);
    std::vector<uint8_t> output;

    bool bfinal = false;

    while (!bfinal) {
        bfinal = reader.read_bits(1) == 1;
        uint32_t btype = reader.read_bits(2);

        if (btype == 0) {
            // Stored (uncompressed) block
            reader.align_to_byte();
            uint16_t len = static_cast<uint16_t>(reader.read_bits(16));
            [[maybe_unused]] uint16_t nlen = static_cast<uint16_t>(reader.read_bits(16));
            for (uint16_t i = 0; i < len; ++i) {
                output.push_back(static_cast<uint8_t>(reader.read_bits(8)));
            }
        } else if (btype == 1 || btype == 2) {
            // Huffman-compressed block
            std::vector<HuffmanNode> litlen_tree;
            std::vector<HuffmanNode> dist_tree;

            if (btype == 1) {
                // Fixed Huffman
                litlen_tree = huffman_build_decode_tree(fixed_litlen_lengths());
                dist_tree = huffman_build_decode_tree(fixed_distance_lengths());
            } else {
                // Dynamic Huffman
                read_dynamic_trees(reader, litlen_tree, dist_tree);
            }

            while (true) {
                int sym = read_huffman_symbol(reader, litlen_tree);

                if (sym < 256) {
                    output.push_back(static_cast<uint8_t>(sym));
                } else if (sym == 256) {
                    break; // end of block
                } else {
                    // Length-distance pair
                    int lidx = sym - 257;
                    uint16_t length = kLengthTable[lidx].base;
                    if (kLengthTable[lidx].extra_bits > 0) {
                        length += static_cast<uint16_t>(
                            reader.read_bits(kLengthTable[lidx].extra_bits));
                    }

                    int dsym = read_huffman_symbol(reader, dist_tree);
                    uint16_t distance = kDistanceTable[dsym].base;
                    if (kDistanceTable[dsym].extra_bits > 0) {
                        distance += static_cast<uint16_t>(
                            reader.read_bits(kDistanceTable[dsym].extra_bits));
                    }

                    // Copy from output buffer
                    std::size_t start = output.size() - distance;
                    for (uint16_t i = 0; i < length; ++i) {
                        output.push_back(output[start + i]);
                    }
                }
            }
        } else {
            throw std::runtime_error("deflate: invalid block type 3");
        }
    }

    return output;
}

} // namespace compression
