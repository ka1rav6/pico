#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Pico::cli {

// --- Spec types (declarative, one per command) ---------------------------

enum class OptionKind : uint8_t {
    BOOLEAN_FLAG, // --release (no value)
    VALUE_FLAG,   // --jobs 4 (takes a value)
};

struct PositionalSpec {
    std::string name;
    bool required;
};

struct FlagSpec {
    std::string long_name;
    std::string short_name;
    std::string help;
};

struct OptionSpec {
    std::string long_name;
    std::string short_name;
    std::string default_value;
    std::string help;
};

struct ArgSpec {
    std::string name;
    std::string summary;
    std::vector<PositionalSpec> positionals;
    std::vector<FlagSpec> flags;
    std::vector<OptionSpec> options;
};

// --- Parsed (result) types ------------------------------------------------

struct Option {
    OptionKind kind;
    std::string name;
    std::optional<std::string> value;
    bool is_short;

    Option(OptionKind kind, std::string name,
           std::optional<std::string> value = std::nullopt,
           bool is_short = false)
        : kind(kind), name(std::move(name)), value(std::move(value)),
          is_short(is_short) {}

    bool operator==(const Option &that) const {
        return kind == that.kind && name == that.name && value == that.value &&
               is_short == that.is_short;
    }
};

struct Arguments {
    std::vector<Option> options;
    std::vector<std::string> positionals;
    std::vector<std::string> passthrough;

    bool has(const std::string &name) const;
    std::optional<std::string> value(const std::string &name) const;
};

struct ParseError {
    std::string message;
    std::string hint;
};

// --- Parsing / help ---------------------------------------------------------

// Parses argv against `spec`. Returns std::nullopt (with `error` filled) on
// failure. `--help`/`-h` are always recognized and surfaced as a `help` flag
// in the result; everything after `--` is captured in `passthrough`.
std::optional<Arguments> parse(const ArgSpec &spec, int argc, const char **argv,
                               ParseError &error);

void print_help(const ArgSpec &spec);

} // namespace Pico::cli
