# Design Doc

## What does pico aim to solve?

Pico is an extremely fast cargo-like build system (and eventually package manager) for C++ projects.
It eliminates the need for CMake and Make configurations entirely.
The goal is to be simple, straightforward, zero-config by default, and useful for anyone — from beginners to power users.

## Core principles

1. **Open-source** — always free and transparent.
2. **Zero config by default** — `pico build` just works. No CMakeLists.txt, no Makefile.
3. **Approachable and casual** — easy for a beginner to use *and* understand the codebase.
4. **Incremental by design** — only rebuild changed files and their transitive dependents.
5. **Clear diagnostics** — meaningful output, verbose when asked, silent otherwise.
6. **Extensible** — easy config (`pico.toml`) and debugging (`pico explain`).

## Language & Build

- **Language:** C++20
- **Build system:** CMake (dogfooding will come once pico can build itself)
- **Dependencies:** Blake3 (git submodule, C library for fast hashing)
- **Config parsing:** Custom TOML lexer/parser written in C (in `src/utils/toml_parser/`)

## Project structure

```tree
├── CMakeLists.txt              # CMake build definition
├── ROADMAP.md                  # Phased development roadmap
├── design-doc.md               # This file
├── list_of_thought_commands.md # CLI command reference
├── README.md
├── CONTRIBUTING.md
├── src/
│   ├── main.cpp                # Entry point
│   ├── cli/                    # CLI dispatch and argument parsing
│   │   ├── cli_entry.h
│   │   └── cli_entry.cpp
│   ├── init/                   # `pico init` — project scaffolding
│   │   ├── init.cpp
│   │   ├── config.toml         # Template for generated pico.toml
│   │   └── pico_gitignore.txt
│   ├── scanner/                # Recursive file walk + hashing
│   │   ├── scanner.hpp
│   │   └── scanner.cpp
│   ├── utils/
│   │   ├── hasher.hpp          # Blake3 wrapper, File, Hash, HashedFileSystem
│   │   ├── hasher.cpp
│   │   └── toml_parser/        # Full TOML lexer + parser (C)
│   ├── diagnostics/            # Event counters and output
│   │   ├── event.hpp
│   │   ├── counter.hpp
│   │   ├── output.hpp
│   │   └── output.cpp
│   ├── cache/                  # (placeholder) Persistent build cache
│   ├── config/                 # (placeholder) pico.toml config reader
│   ├── compiler/               # (placeholder) Compiler invocation
│   ├── graph/                  # (placeholder) #include dependency analysis
│   ├── scheduler/              # (placeholder) Parallel build scheduling
│   ├── linker/                 # (placeholder) Object file linking
│   ├── runner/                 # (placeholder) Binary execution
│   ├── doctor/                 # (placeholder) Environment diagnostics
│   └── fmt/                    # (placeholder) Code formatting
├── tests/                      # Test suite (empty, to be filled)
├── benchmarks/                 # Performance benchmarks (empty, to be filled)
└── docs/                       # Documentation (empty, to be filled)
```

Every directory is named after the concept it implements, making the codebase easy to navigate.

## Build lifecycle

```
pico build
    │
    ├─ main.cpp ──► cli/ ──► dispatch "build"
    │
    ├─ scanner/ ──► recursively walk src/, hash every .cpp/.h file
    │
    ├─ cache/   ──► compare hashes against previous build; mark changed files
    │
    ├─ graph/   ──► parse #include directives; build dependency DAG
    │               mark transitive dependents of changed files
    │
    ├─ scheduler ──► topological sort; produce parallel compile plan
    │
    ├─ compiler/ ──► spawn compiler (clang++ / g++) N threads at a time
    │
    └─ linker/  ──► link compiled objects into final binary/library
```

## Module responsibilities

| Module | Status | Responsibility |
|--------|--------|----------------|
| `cli/` | ✅ Partial | Parse CLI args, dispatch to the right command handler |
| `init/` | ✅ Partial | `pico init` — scaffold a new project |
| `utils/` | ✅ Mostly done | Blake3 hasher, TOML parser, file system helpers |
| `scanner/` | ✅ Partial | Walk directory tree, hash source files |
| `diagnostics/` | ✅ Partial | Event counters, summary printing |
| `cache/` | ❌ TBD | Persistent hash map: file → last-known hash |
| `config/` | ❌ TBD | Read `pico.toml`, expose build configuration |
| `graph/` | ❌ TBD | Extract `#include` graph, build DAG |
| `scheduler/` | ❌ TBD | Topological sort, thread pool management |
| `compiler/` | ❌ TBD | Invoke compiler, manage compilation units |
| `linker/` | ❌ TBD | Link object files into executables / libraries |
| `runner/` | ❌ TBD | Build and run executables with arg passthrough |
| `doctor/` | ❌ TBD | Verify toolchain and environment |
| `fmt/` | ❌ TBD | Run code formatter across the project |

## CLI commands (planned)

| Command | Phase | Description |
|---------|-------|-------------|
| `pico init` | 0 | Scaffold a new project |
| `pico build` | 1 | Compile and link the project |
| `pico add` | 2 | Add a dependency |
| `pico remove` | 2 | Remove a dependency |
| `pico update` | 2 | Update dependencies |
| `pico run` | 3 | Build and run |
| `pico test` | 3 | Build and run tests |
| `pico clean` | 3 | Remove build artifacts |
| `pico doctor` | 3 | Check environment setup |
| `pico fmt` | 3 | Format source code |
| `pico check` | 3 | Static analysis, no binary |
| `pico explain` | 3 | Show why files were rebuilt |
| `pico graph` | 3 | Visualize dependency graph |

See [ROADMAP.md](ROADMAP.md) for the full phased development plan.

