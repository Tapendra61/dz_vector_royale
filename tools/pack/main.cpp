// vector_pack — bundles a directory tree into a single .pak archive.
//
// Format (little-endian, all platforms):
//   magic       : char[8]   = "VECTPAK\0"
//   version     : uint32    = 1
//   entry_count : uint32
//   entries     : entry_count * {
//       name_length : uint16
//       name        : char[name_length]   (forward-slash separated, relative)
//       size        : uint64
//       offset      : uint64
//   }
//   blob        : raw concatenated file contents
//
// Mobile platforms (Phase 8) will read this archive directly from the APK /
// IPA bundle instead of touching the filesystem — same format, same reader.

#include "core/logging.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct Entry {
    std::string name;
    fs::path    source;
    std::uint64_t size   = 0;
    std::uint64_t offset = 0;
};

template <typename T>
void write_le(std::ostream& out, T value) {
    static_assert(std::is_integral_v<T>);
    char buf[sizeof(T)];
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        buf[i] = static_cast<char>((value >> (i * 8)) & 0xFF);
    }
    out.write(buf, sizeof(T));
}

bool collect(const fs::path& root, std::vector<Entry>& out) {
    if (!fs::is_directory(root)) {
        std::fprintf(stderr, "pack: not a directory: %s\n", root.string().c_str());
        return false;
    }
    for (auto it = fs::recursive_directory_iterator(root); it != fs::recursive_directory_iterator(); ++it) {
        if (!it->is_regular_file()) continue;
        const auto rel = fs::relative(it->path(), root).generic_string();
        if (rel.empty() || rel.front() == '.') continue;
        Entry e;
        e.name   = rel;
        e.source = it->path();
        e.size   = fs::file_size(it->path());
        out.push_back(std::move(e));
    }
    std::sort(out.begin(), out.end(),
              [](const Entry& a, const Entry& b) { return a.name < b.name; });
    return true;
}

void print_usage() {
    std::puts("usage: vector_pack <assets-dir> <output.pak>");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        print_usage();
        return 1;
    }
    const fs::path assets_dir = argv[1];
    const fs::path output     = argv[2];

    std::vector<Entry> entries;
    if (!collect(assets_dir, entries)) return 1;

    // Pre-compute offsets: header + index + blob.
    std::uint64_t index_size = 0;
    for (const auto& e : entries) {
        index_size += 2 + e.name.size() + 8 + 8;
    }
    constexpr std::uint64_t header_size = 8 /*magic*/ + 4 /*version*/ + 4 /*count*/;
    std::uint64_t cursor = header_size + index_size;
    for (auto& e : entries) {
        e.offset  = cursor;
        cursor   += e.size;
    }

    std::ofstream out(output, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        std::fprintf(stderr, "pack: cannot open output %s\n", output.string().c_str());
        return 1;
    }

    out.write("VECTPAK\0", 8);
    write_le<std::uint32_t>(out, 1);
    write_le<std::uint32_t>(out, static_cast<std::uint32_t>(entries.size()));

    for (const auto& e : entries) {
        write_le<std::uint16_t>(out, static_cast<std::uint16_t>(e.name.size()));
        out.write(e.name.data(), static_cast<std::streamsize>(e.name.size()));
        write_le<std::uint64_t>(out, e.size);
        write_le<std::uint64_t>(out, e.offset);
    }

    std::vector<char> buf(64 * 1024);
    for (const auto& e : entries) {
        std::ifstream in(e.source, std::ios::binary);
        if (!in.is_open()) {
            std::fprintf(stderr, "pack: cannot open %s\n", e.source.string().c_str());
            return 1;
        }
        while (in) {
            in.read(buf.data(), static_cast<std::streamsize>(buf.size()));
            out.write(buf.data(), in.gcount());
        }
    }

    std::printf("pack: %zu files -> %s\n", entries.size(), output.string().c_str());
    return 0;
}
