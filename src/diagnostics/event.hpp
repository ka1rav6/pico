#pragma once

#include <cstddef>
#include <cstdint>

namespace Pico {

enum class Event : uint8_t {
  file_hash_generated,
  file_recompiled,
  file_dependency_checked,
  file_new_compiled,
  graph_new_nodes_created,
  graph_structure_modified,
  directory_scanned,
  COUNT
};

inline constexpr size_t EVENT_COUNT = static_cast<size_t>(Event::COUNT);

inline constexpr const char *event_name(Event e) {
  switch (e) {
  case Event::file_hash_generated:
    return "file_hash_generated";
  case Event::file_recompiled:
    return "file_recompiled";
  case Event::file_dependency_checked:
    return "file_dependency_checked";
  case Event::file_new_compiled:
    return "file_new_compiled";
  case Event::graph_new_nodes_created:
    return "graph_new_nodes_created";
  case Event::graph_structure_modified:
    return "graph_structure_modified";
  case Event::directory_scanned:
    return "directory_scanned";
  default:
    return "unknown";
  }
}

} // namespace Pico
