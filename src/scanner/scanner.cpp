
#include "scanner.hpp"

namespace Pico {
static FileType get_file_type(const std::filesystem::path &f) {
  auto ext = f.extension();
  if (ext == ".cpp" || ext == ".cc" || ext == ".cxx")
    return CPP_FILE;
  return HEADER;
}
HashedFileSystem scan(const std::filesystem::path &dir) {
  HashedFileSystem system;
  for (const auto &entry : std::filesystem::recursive_directory_iterator(dir)) {
    if (entry.is_regular_file()) {
      auto path = entry.path();
      auto ext = path.extension();
      if (ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".h" ||
          ext == ".hpp" || ext == ".hh" || ext == ".hxx") {
        auto type = get_file_type(path);
        auto hash_str = Hasher::to_hex(Hasher::hash_file(path));
        system.map.emplace(File(type, path), hash_str);
      }
    }
  }
  return system;
}
} // namespace Pico
