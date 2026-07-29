#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace Pico {

using Hash = std::array<uint8_t, 32>;

class Hasher {
public:
  static Hash hash_file(const std::filesystem::path &path);
  static std::string to_hex(const Hash &hash);
  static bool equal(const Hash &a, const Hash &b);
};

enum FileType { HEADER, CPP_FILE };
struct File {
  FileType type;
  std::filesystem::path file;
  File(FileType t, std::filesystem::path f) : type(t), file(std::move(f)) {}
  File() = default;
  bool operator==(const File &other) const {
    return type == other.type && file == other.file;
  }
};

} // namespace Pico

template<>
struct std::hash<Pico::File> {
  size_t operator()(const Pico::File &f) const noexcept {
    return std::hash<int>()(f.type) ^
           (std::hash<std::string>()(f.file.string()) << 1);
  }
};

namespace Pico {

typedef std::unordered_map<File, std::string> HashMap;
class HashedFileSystem {
public:
  HashMap map;
  void to_file(std::filesystem::path file);
  HashedFileSystem() = default;
};

} // namespace Pico
