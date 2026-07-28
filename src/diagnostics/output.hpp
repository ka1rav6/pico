#pragma once

#include "counter.hpp"
#include <string>

namespace Pico {

void print_summary(const Counter &counter);
int write_to_file(const Counter &counter, const std::string &path);

} // namespace Pico
