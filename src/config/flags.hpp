#pragma once

#include <cstdint>

namespace Pico {

// Bitmask of boolean compiler/diagnostics toggles read from config.toml.
enum class CompileFlags : uint64_t {
    TREAT_WARNINGS_AS_ERRORS = 1ULL << 0,
    ENABLE_ALL_WARNINGS = 1ULL << 1,
    ENABLE_EXTRA_WARNINGS = 1ULL << 2,
    ENABLE_PEDANTIC = 1ULL << 3,
    FATAL_ERRORS = 1ULL << 4,
    COLOR_DIAGNOSTICS = 1ULL << 5,
    LINK_TIME_OPTIMIZATION = 1ULL << 6,
    FAST_MATH = 1ULL << 7,
    OMIT_FRAME_POINTER = 1ULL << 8,
    DEBUG_SYMBOLS = 1ULL << 9,
    STRIP_SYMBOLS = 1ULL << 10,
    GNU_EXTENSIONS = 1ULL << 11,
    EXCEPTIONS = 1ULL << 12,
    RTTI = 1ULL << 13,
    THREADSAFE_STATICS = 1ULL << 14,
    ADDRESS_SANITIZER = 1ULL << 15,
    THREAD_SANITIZER = 1ULL << 16,
    UNDEFINED_SANITIZER = 1ULL << 17,
    MEMORY_SANITIZER = 1ULL << 18,
    LEAK_SANITIZER = 1ULL << 19,
    NATIVE_ARCHITECTURE = 1ULL << 20,
    STATIC = 1ULL << 21,
    SHARED = 1ULL << 22,
    PIE = 1ULL << 23,
    POSITION_INDEPENDENT_CODE = 1ULL << 24,
    VERBOSE = 1ULL << 25,
    SAVE_TEMPS = 1ULL << 26,
    EMIT_ASSEMBLY = 1ULL << 27,
    COMPILE_ONLY = 1ULL << 28,
};

} // namespace Pico
