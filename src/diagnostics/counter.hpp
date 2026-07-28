#pragma once

#include "event.hpp"
#include <array>
#include <cstdint>

namespace Pico {

struct Counter {
  std::array<uint32_t, EVENT_COUNT> counts{};

  void reset() { counts.fill(0); }

  void increment(Event event) { counts[static_cast<size_t>(event)] += 1; }

  uint32_t get(Event event) const { return counts[static_cast<size_t>(event)]; }
};

} // namespace Pico
