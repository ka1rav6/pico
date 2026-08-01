#include "read_config.hpp"

#include "flags.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

namespace Pico {
namespace {

// true when the node exists and holds a TOML boolean value.
bool get_bool(const TOML::Table &table, const std::string &key) {
    TOML::Node node = table.node(key);
    return node.valid() && static_cast<bool>(node);
}

void set_flag(uint64_t &flags, CompileFlags flag, bool on) {
    if (on)
        flags |= static_cast<uint64_t>(flag);
}

} // namespace

TOML::Toml read_config(std::filesystem::path filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        std::cerr << "Could not read config.toml file. Do you want to "
                     "initialise it first?"
                  << std::endl;
        exit(EXIT_FAILURE);
    }
    file.seekg(0, std::ios::end);
    std::size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string contents(size, '\0');
    file.read(contents.data(), size);
    return TOML::Toml(contents);
}

uint64_t get_compile_flags(const TOML::Toml &toml) {
    uint64_t flags = 0;
    const TOML::Table compiler = toml.table("compiler");
    const TOML::Table diagnostics = toml.table("diagnostics");
    // Diagnostics
    set_flag(flags, CompileFlags::TREAT_WARNINGS_AS_ERRORS,
             get_bool(diagnostics, "treat_warnings_as_errors"));
    set_flag(flags, CompileFlags::ENABLE_ALL_WARNINGS,
             get_bool(diagnostics, "enable_all_warnings"));
    set_flag(flags, CompileFlags::ENABLE_EXTRA_WARNINGS,
             get_bool(diagnostics, "enable_extra_warnings"));
    set_flag(flags, CompileFlags::ENABLE_PEDANTIC,
             get_bool(diagnostics, "enable_pedantic"));
    set_flag(flags, CompileFlags::FATAL_ERRORS,
             get_bool(diagnostics, "fatal_errors"));
    set_flag(flags, CompileFlags::COLOR_DIAGNOSTICS,
             get_bool(diagnostics, "color_diagnostics"));

    // Optimization
    set_flag(flags, CompileFlags::LINK_TIME_OPTIMIZATION,
             get_bool(compiler, "link_time_optimization"));
    set_flag(flags, CompileFlags::FAST_MATH, get_bool(compiler, "fast_math"));
    set_flag(flags, CompileFlags::OMIT_FRAME_POINTER,
             get_bool(compiler, "omit_frame_pointer"));

    // Debugging
    set_flag(flags, CompileFlags::DEBUG_SYMBOLS,
             get_bool(compiler, "debug_symbols"));
    set_flag(flags, CompileFlags::STRIP_SYMBOLS,
             get_bool(compiler, "strip_symbols"));

    // C++ standard
    set_flag(flags, CompileFlags::GNU_EXTENSIONS,
             get_bool(compiler, "gnu_extensions"));

    // Runtime
    set_flag(flags, CompileFlags::EXCEPTIONS, get_bool(compiler, "exceptions"));
    set_flag(flags, CompileFlags::RTTI, get_bool(compiler, "rtti"));
    set_flag(flags, CompileFlags::THREADSAFE_STATICS,
             get_bool(compiler, "threadsafe_statics"));

    // Sanitizers
    set_flag(flags, CompileFlags::ADDRESS_SANITIZER,
             get_bool(compiler, "address_sanitizer"));
    set_flag(flags, CompileFlags::THREAD_SANITIZER,
             get_bool(compiler, "thread_sanitizer"));
    set_flag(flags, CompileFlags::UNDEFINED_SANITIZER,
             get_bool(compiler, "undefined_sanitizer"));
    set_flag(flags, CompileFlags::MEMORY_SANITIZER,
             get_bool(compiler, "memory_sanitizer"));
    set_flag(flags, CompileFlags::LEAK_SANITIZER,
             get_bool(compiler, "leak_sanitizer"));
    // CPU
    set_flag(flags, CompileFlags::NATIVE_ARCHITECTURE,
             get_bool(compiler, "native_architecture"));
    // Linking
    set_flag(flags, CompileFlags::STATIC, get_bool(compiler, "static"));
    set_flag(flags, CompileFlags::SHARED, get_bool(compiler, "shared"));
    set_flag(flags, CompileFlags::PIE, get_bool(compiler, "pie"));
    set_flag(flags, CompileFlags::POSITION_INDEPENDENT_CODE,
             get_bool(compiler, "position_independent_code"));

    // Misc
    set_flag(flags, CompileFlags::VERBOSE, get_bool(compiler, "verbose"));
    set_flag(flags, CompileFlags::SAVE_TEMPS, get_bool(compiler, "save_temps"));
    set_flag(flags, CompileFlags::EMIT_ASSEMBLY,
             get_bool(compiler, "emit_assembly"));
    set_flag(flags, CompileFlags::COMPILE_ONLY,
             get_bool(compiler, "compile_only"));
    return flags;
}

} // namespace Pico
