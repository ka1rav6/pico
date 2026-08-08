#include "cli_entry.h"

#include <iostream>
#include <string>

#include "argparser.hpp"
#include "specs.hpp"

namespace Pico {
namespace {

void print_usage() {
    std::cout << "pico - an extremely fast, cargo-like build system for C++\n\n"
              << "usage: pico <command> [args]\n\n"
              << "Commands:\n";
    for (const auto *spec : cli::all_specs())
        std::cout << "  " << spec->name << "\t" << spec->summary << "\n";
    std::cout << "\nRun `pico help <command>` for details.\n";
}

} // namespace

void handle_cli(int argc, const char **argv) {
    if (argc < 2) {
        print_usage();
        return;
    }

    std::string command = argv[1];

    const cli::ArgSpec *spec = cli::spec_for(command);
    if (spec == nullptr) {
        std::cerr << "pico: unknown command `" << command
                  << "`\n  see `pico help` for the command list\n";
        return;
    }

    cli::ParseError error;
    auto args = cli::parse(*spec, argc - 1, argv + 1, error);
    if (!args) {
        std::cerr << "pico " << command << ": " << error.message;
        if (!error.hint.empty())
            std::cerr << "\n  " << error.hint;
        std::cerr << "\n";
        return;
    }

    if (args->has("help")) {
        cli::print_help(*spec);
        return;
    }

    if (command == "help") {
        if (args->positionals.empty()) {
            print_usage();
            return;
        }
        const cli::ArgSpec *target = cli::spec_for(args->positionals[0]);
        if (target == nullptr) {
            std::cerr << "pico: unknown command `" << args->positionals[0]
                      << "`\n";
            return;
        }
        cli::print_help(*target);
        return;
    }

    std::cout << "pico " << command
              << ": command handler not implemented yet\n";
}

} // namespace Pico
