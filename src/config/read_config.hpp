#pragma once

#include "../utils/toml_parser/toml.hpp"

#include <cstdint>
#include <filesystem>

namespace Pico {

TOML::Toml read_config(std::filesystem::path filepath);
uint64_t get_compile_flags(const TOML::Toml &toml);

} // namespace Pico
