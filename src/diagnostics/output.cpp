#include "output.hpp"

#include <fstream>
#include <iostream>

namespace Pico {

void print_summary(const Counter &counter) {
  for (size_t i = 0; i < EVENT_COUNT; ++i) {
    auto event = static_cast<Event>(i);
    uint32_t count = counter.get(event);
    if (count > 0) {
      std::cout << event_name(event) << ": " << count << "\n";
    }
  }
}

int write_to_file(const Counter &counter, const std::string &path) {
  std::ofstream file(path);
  if (!file.is_open()) {
    return -1;
  }

  for (size_t i = 0; i < EVENT_COUNT; ++i) {
    auto event = static_cast<Event>(i);
    uint32_t count = counter.get(event);
    file << event_name(event) << ": " << count << "\n";
  }

  file.flush();
  return 0;
}

} // namespace Pico
