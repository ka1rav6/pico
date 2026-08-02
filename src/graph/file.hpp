#pragma once

#include "../utils/hasher.hpp"
#include <filesystem>
#include <vector>

namespace Pico::Graph {
class File {
public:
    std::filesystem::path path;
    HASH::FileType type;
    HASH::Hash hash;
    bool toCompile;
    std::vector<struct File *> dep_list;

    File(std::filesystem::path path, HASH::FileType type, bool toCompile = true)
        : path(path), type(type), toCompile(toCompile) {}
    ~File();

    HASH::Hash _hash() { return HASH::Hasher::hash_file(path); }
};
} // namespace Pico::Graph
