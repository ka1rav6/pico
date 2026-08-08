#include "specs.hpp"

#include <utility>

namespace Pico::cli {
namespace {

const ArgSpec kInit = {
    .name = "init",
    .summary = "Initialize a new Pico project",
    .positionals = {{"directory", false}},
    .flags =
        {
            {"git", "", "initialize a git repository"},
            {"clang", "", "use clang as the default compiler"},
            {"gcc", "", "use gcc as the default compiler"},
        },
    .options = {},
};

const ArgSpec kBuild = {
    .name = "build",
    .summary = "Build the project",
    .positionals = {},
    .flags =
        {
            {"release", "", "switch to the release profile"},
            {"clean", "", "wipe cache + build dir before building"},
            {"verbose", "", "full compiler command lines + per-stage timing"},
            {"keep-going", "", "don't abort after a failed compile"},
        },
    .options =
        {
            {"jobs", "j", "",
             "compile thread count (default: hardware concurrency)"},
        },
};

const ArgSpec kRun = {
    .name = "run",
    .summary = "Build (if required) and run an executable",
    .positionals = {{"target", false}},
    .flags =
        {
            {"release", "", "switch to the release profile"},
        },
    .options = {},
};

const ArgSpec kTest = {
    .name = "test",
    .summary = "Build and run all tests",
    .positionals = {},
    .flags =
        {
            {"release", "", "switch to the release profile"},
            {"verbose", "", "print per-test details"},
        },
    .options = {},
};

const ArgSpec kClean = {
    .name = "clean",
    .summary = "Remove generated build files",
    .positionals = {},
    .flags =
        {
            {"cache", "", "also wipe the incremental build cache"},
        },
    .options = {},
};

const ArgSpec kAdd = {
    .name = "add",
    .summary = "Add a dependency to the project",
    .positionals = {{"package", true}},
    .flags = {},
    .options = {},
};

const ArgSpec kRemove = {
    .name = "remove",
    .summary = "Remove a dependency from the project",
    .positionals = {{"package", true}},
    .flags = {},
    .options = {},
};

const ArgSpec kUpdate = {
    .name = "update",
    .summary = "Update project dependencies",
    .positionals = {},
    .flags =
        {
            {"latest", "",
             "re-resolve against the newest baseline, not the pinned one"},
        },
    .options = {},
};

const ArgSpec kExplain = {
    .name = "explain",
    .summary = "Explain what happened during the previous build",
    .positionals = {},
    .flags = {},
    .options = {},
};

const ArgSpec kGraph = {
    .name = "graph",
    .summary = "Display the project's dependency graph",
    .positionals = {},
    .flags =
        {
            {"dot", "", "export Graphviz format"},
        },
    .options =
        {
            {"focus", "", "", "only show the subgraph relevant to one file"},
        },
};

const ArgSpec kDoctor = {
    .name = "doctor",
    .summary =
        "Check whether the development environment is correctly configured",
    .positionals = {},
    .flags = {},
    .options = {},
};

const ArgSpec kFmt = {
    .name = "fmt",
    .summary = "Format the project's source code",
    .positionals = {},
    .flags =
        {
            {"check", "", "fail without modifying files (CI use)"},
        },
    .options = {},
};

const ArgSpec kCheck = {
    .name = "check",
    .summary = "Run compiler diagnostics without producing executables",
    .positionals = {},
    .flags = {},
    .options = {},
};

const ArgSpec kImport = {
    .name = "import",
    .summary = "Import project structure from an existing build system",
    .positionals = {{"source", true}},
    .flags =
        {
            {"emit-toml", "",
             "write an equivalent pico.toml instead of building directly"},
        },
    .options = {},
};

const ArgSpec kHelp = {
    .name = "help",
    .summary = "Show help for a command",
    .positionals = {{"command", false}},
    .flags = {},
    .options = {},
};

const std::vector<const ArgSpec *> kAllSpecs = {
    &kInit,    &kBuild, &kRun,    &kTest, &kClean, &kAdd,    &kRemove, &kUpdate,
    &kExplain, &kGraph, &kDoctor, &kFmt,  &kCheck, &kImport, &kHelp,
};

} // namespace

const ArgSpec *spec_for(const std::string &name) {
    for (const auto *s : kAllSpecs)
        if (s->name == name)
            return s;
    return nullptr;
}

std::vector<const ArgSpec *> all_specs() { return kAllSpecs; }

} // namespace Pico::cli
