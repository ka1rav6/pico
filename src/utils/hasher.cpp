#include "hasher.hpp"

#include <blake3.h> // submodule for hashing

#include <fstream>
#include <iomanip>
#include <sstream>

namespace Pico {

// reads a file through the buffer and hashes it.
// returns the hash
// Note: the hash is just an std::array of uint8_t
Hash Hasher::hash_file(const std::filesystem::path &path) {
  constexpr size_t BUFFER_SIZE = 64 * 1024;
  std::ifstream file(path, std::ios::binary);
  if (!file)
    throw std::runtime_error("Cannot open file: " + path.string());
  blake3_hasher hasher;
  blake3_hasher_init(&hasher);

  std::array<char, BUFFER_SIZE> buffer;

  while (file) {
    file.read(buffer.data(), BUFFER_SIZE);
    std::streamsize read = file.gcount();
    if (read > 0)
      blake3_hasher_update(&hasher, buffer.data(), static_cast<size_t>(read));
  }
  Hash digest;
  blake3_hasher_finalize(&hasher, digest.data(), digest.size());
  return digest;
}

std::string Hasher::to_hex(const Hash &hash) {
  std::ostringstream ss;

  for (uint8_t byte : hash) {
    ss << std::hex << std::setw(2) << std::setfill('0')
       << static_cast<int>(byte);
  }
  return ss.str();
}
bool Hasher::equal(const Hash &a, const Hash &b) { return a == b; }
} // namespace Pico
