
#include "scanner.hpp"

namespace Pico {
static HASH::FileType get_file_type(const std::filesystem::path &f) {
    auto ext = f.extension();
    if (ext == ".cpp" || ext == ".cc" || ext == ".cxx")
        return HASH::CPP_FILE;
    return HASH::HEADER;
}
HASH::HashedFileSystem scan(const std::filesystem::path &dir) {
    HASH::HashedFileSystem system;
    for (const auto &entry :
         std::filesystem::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            auto path = entry.path();
            auto ext = path.extension();
            if (ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".h" ||
                ext == ".hpp" || ext == ".hh" || ext == ".hxx") {
                auto type = get_file_type(path);
                auto hash_str =
                    HASH::Hasher::to_hex(HASH::Hasher::hash_file(path));
                system.map.emplace(HASH::File(type, path), hash_str);
            }
        }
    }
    return system;
}
} // namespace Pico
