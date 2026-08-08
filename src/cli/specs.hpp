#pragma once

#include <string>
#include <vector>

#include "argparser.hpp"

namespace Pico::cli {

const ArgSpec *spec_for(const std::string &name);
std::vector<const ArgSpec *> all_specs();

} // namespace Pico::cli
