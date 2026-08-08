#pragma once

#include "../utils/hasher.hpp"

namespace Pico {
HASH::HashedFileSystem scan(std::filesystem::path);
} // namespace Pico
