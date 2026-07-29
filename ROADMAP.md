# Pico Roadmap

> **Current phase: Phase 0 — Scaffolding ~80% complete**

---

## Phase 0: Foundation (Scaffolding)

- [x] Project pivot from Zig to C++
- [x] CMake build system (C++20)
- [x] Blake3 integration (submodule)
- [x] CLI dispatcher (`main.cpp` → `cli_entry.cpp`)
- [x] `pico init` — project scaffolding (git init, dirs, config.toml, .gitignore)
- [x] Scanner — recursive file walk + Blake3 hashing
- [x] Hasher — Blake3-based file hashing utilities
- [x] TOML lexer/parser (C, full implementation)
- [x] Diagnostics — event counters and output printing
- [ ] CLI argument parsing (flags, options, subcommands)
- [ ] Error handling infrastructure
- [ ] Logging / verbose output infrastructure

---

## Phase 1: Core Build Pipeline

**Goal:** `pico build` actually compiles and links a project.

- [ ] **Config** — read and validate `pico.toml` via TOML parser; expose build config (compiler, std, flags, etc.)
- [ ] **Cache** — persistent cache of file hashes → skip unchanged files on rebuild; invalidate on hash change
- [ ] **Graph** — `#include` dependency analysis; build a DAG of source files
- [ ] **Scheduler** — topological sort of the dependency graph; determine parallel compile order
- [ ] **Compiler** — invoke the C++ compiler (clang++/g++) on compilation units; manage object file output
- [ ] **Linker** — link compiled objects into executable(s) / shared library(ies)
- [ ] **Build command** — wire scanner → cache → graph → scheduler → compiler → linker into the `pico build` command
- [ ] **`--release` / `--clean` / `--verbose` / `-j` flags** for `pico build`

---

## Phase 2: Package Management

**Goal:** `pico add fmt` downloads and makes a library available.

- [ ] Package registry format and discovery
- [ ] Download and caching of source dependencies
- [ ] `pico add <package>` — download, store in `libs/`, update `pico.toml`
- [ ] `pico remove <package>` — remove from config, clean cache
- [ ] `pico update` — check for newer versions, update lock file
- [ ] Dependency resolution (version constraints, transitive deps)
- [ ] Lock file (`pico.lock`) for reproducible builds

---

## Phase 3: Developer Experience

**Goal:** Rich CLI with test running, formatting, analysis, and introspection.

- [ ] `pico run` — build + execute, with argument passthrough
- [ ] `pico test` — discover, build, and run tests; print summary
- [ ] `pico clean` — remove build artifacts, optional cache clearing
- [ ] `pico doctor` — verify compiler, linker, debugger, project config, cache integrity
- [ ] `pico fmt` — run clang-format (or configured formatter) across the project
- [ ] `pico check` — compiler diagnostics + static analysis, no binary output
- [ ] `pico explain` — show what was rebuilt, why, cache hits/misses, timings
- [ ] `pico graph` — visualize dependency graph (with `--dot` export)

---

## Phase 4: Production Readiness

**Goal:** Fast, reliable, cross-platform, well-documented.

- [ ] Cross-platform support (Linux, macOS, Windows)
- [ ] Comprehensive test suite
- [ ] Benchmarks vs CMake + Make / Ninja
- [ ] Continuous integration (GitHub Actions)
- [ ] Precompiled headers (PCH) support
- [ ] Distributed / remote build caching (e.g., sccache-style)
- [ ] IDE integration (compile_commands.json, LSP support)
- [ ] Community documentation and contribution guides
- [ ] Releases and binary distribution
