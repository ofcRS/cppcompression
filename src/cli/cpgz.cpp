#include "gzip/gzip.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <span>
#include <vector>

namespace fs = std::filesystem;

// ── Options ────────────────────────────────────────────────────────────────

struct Options {
    bool decompress = false;
    bool keep       = false;
    bool force      = false;
    bool to_stdout  = false;
    bool verbose    = false;
    bool list       = false;
    std::vector<fs::path> files;
};

static void print_usage(const char* prog) {
    std::cerr
        << "Usage: " << prog << " [OPTIONS] FILE...\n"
        << "\n"
        << "Compress or decompress files in gzip format (RFC 1952).\n"
        << "\n"
        << "Options:\n"
        << "  -d, --decompress   Decompress mode\n"
        << "  -k, --keep         Keep original file\n"
        << "  -f, --force        Overwrite existing output file\n"
        << "  -c, --stdout       Write to stdout\n"
        << "  -l, --list         List contents of gzip file(s)\n"
        << "  -v, --verbose      Print compression statistics\n"
        << "  -h, --help         Show this help\n";
}

static std::optional<Options> parse_args(int argc, char* argv[]) {
    Options opts;
    bool stop_flags = false;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (!stop_flags && arg == "--") {
            stop_flags = true;
            continue;
        }

        if (!stop_flags && arg.starts_with("--")) {
            if (arg == "--decompress") opts.decompress = true;
            else if (arg == "--keep")       opts.keep = true;
            else if (arg == "--force")      opts.force = true;
            else if (arg == "--stdout")     opts.to_stdout = true;
            else if (arg == "--list")       opts.list = true;
            else if (arg == "--verbose")    opts.verbose = true;
            else if (arg == "--help") {
                print_usage(argv[0]);
                return std::nullopt;
            } else {
                std::cerr << argv[0] << ": unknown option: " << arg << '\n';
                return std::nullopt;
            }
        } else if (!stop_flags && arg.starts_with('-') && arg.size() > 1) {
            for (std::size_t j = 1; j < arg.size(); ++j) {
                switch (arg[j]) {
                    case 'd': opts.decompress = true; break;
                    case 'k': opts.keep = true; break;
                    case 'f': opts.force = true; break;
                    case 'c': opts.to_stdout = true; break;
                    case 'l': opts.list = true; break;
                    case 'v': opts.verbose = true; break;
                    case 'h':
                        print_usage(argv[0]);
                        return std::nullopt;
                    default:
                        std::cerr << argv[0] << ": unknown flag: -" << arg[j] << '\n';
                        return std::nullopt;
                }
            }
        } else {
            opts.files.emplace_back(argv[i]);
        }
    }

    return opts;
}

// ── File I/O ───────────────────────────────────────────────────────────────

static std::vector<uint8_t> read_file(const fs::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("cannot open: " + path.string());
    }
    auto size = file.tellg();
    file.seekg(0);
    std::vector<uint8_t> buffer(static_cast<std::size_t>(size));
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

static void write_file(const fs::path& path, std::span<const uint8_t> data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("cannot create: " + path.string());
    }
    file.write(reinterpret_cast<const char*>(data.data()),
               static_cast<std::streamsize>(data.size()));
}

static void write_stdout(std::span<const uint8_t> data) {
    std::cout.write(reinterpret_cast<const char*>(data.data()),
                    static_cast<std::streamsize>(data.size()));
}

// ── Path helpers ───────────────────────────────────────────────────────────

static fs::path output_path_for(const fs::path& input, bool decompress) {
    if (decompress) {
        auto ext = input.extension();
        if (ext != ".gz") {
            throw std::runtime_error(input.string() + ": unknown suffix -- ignored");
        }
        return input.parent_path() / input.stem();
    }
    auto out = input;
    out += ".gz";
    return out;
}

// ── Process one file ───────────────────────────────────────────────────────

static bool process_file(const fs::path& input, const Options& opts) {
    try {
        if (!fs::exists(input)) {
            std::cerr << input.string() << ": no such file\n";
            return false;
        }
        if (!fs::is_regular_file(input)) {
            std::cerr << input.string() << ": not a regular file\n";
            return false;
        }

        auto out_path = output_path_for(input, opts.decompress);

        if (!opts.to_stdout && !opts.force && fs::exists(out_path)) {
            std::cerr << out_path.string() << ": already exists; use -f to overwrite\n";
            return false;
        }

        auto data = read_file(input);
        auto result = opts.decompress
            ? compression::gzip_decompress(data)
            : compression::gzip_compress(data);

        if (opts.to_stdout) {
            write_stdout(result);
        } else {
            write_file(out_path, result);
        }

        if (opts.verbose) {
            double ratio = data.empty() ? 0.0
                : 100.0 * static_cast<double>(result.size()) / static_cast<double>(data.size());
            std::cerr << input.string() << ": "
                      << data.size() << " -> " << result.size()
                      << " (" << std::fixed;
            std::cerr.precision(1);
            std::cerr << ratio << "%)\n";
        }

        if (!opts.keep && !opts.to_stdout) {
            fs::remove(input);
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << input.string() << ": " << e.what() << '\n';
        return false;
    }
}

// ── List mode ──────────────────────────────────────────────────────────────

static void print_list_header() {
    std::cout
        << "   compressed  uncompressed  ratio  uncompressed_name\n";
}

static bool list_file(const fs::path& input) {
    try {
        if (!fs::exists(input) || !fs::is_regular_file(input)) {
            std::cerr << input.string() << ": no such file\n";
            return false;
        }

        auto data = read_file(input);
        auto info = compression::gzip_info(data);

        // Ratio is relative to the reported uncompressed size. ISIZE is mod 2^32,
        // so for files ≥ 4GiB the ratio is indicative, not exact.
        double ratio = info.uncompressed_size == 0
            ? 0.0
            : 100.0 * (1.0 - static_cast<double>(data.size())
                              / static_cast<double>(info.uncompressed_size));

        // Prefer the name embedded in the gzip header; fall back to stripping ".gz".
        std::string name = info.original_name;
        if (name.empty()) {
            auto stem = input.extension() == ".gz" ? input.stem() : input.filename();
            name = stem.string();
        }

        std::cout << std::setw(13) << data.size()
                  << std::setw(14) << info.uncompressed_size
                  << ' ' << std::fixed << std::setprecision(1) << std::setw(5) << ratio << "%  "
                  << name << '\n';
        return true;
    } catch (const std::exception& e) {
        std::cerr << input.string() << ": " << e.what() << '\n';
        return false;
    }
}

// ── Main ───────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    auto opts = parse_args(argc, argv);
    if (!opts) return EXIT_FAILURE;

    if (opts->files.empty()) {
        std::cerr << argv[0] << ": no files specified\n";
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    bool all_ok = true;

    if (opts->list) {
        print_list_header();
        for (const auto& file : opts->files) {
            if (!list_file(file)) all_ok = false;
        }
        return all_ok ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    for (const auto& file : opts->files) {
        if (!process_file(file, *opts)) {
            all_ok = false;
        }
    }

    return all_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
