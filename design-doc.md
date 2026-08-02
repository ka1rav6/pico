# Design Doc

## What does pico aim to solve?

Pico is an extremely fast, cargo-like build system (and package manager) for C++ projects.
It eliminates the need for hand-written CMake and Make configurations for day-to-day project
work, while remaining able to **import** project structure from CMake and Premake so it can
be adopted incrementally in existing codebases rather than requiring a rewrite.

## Core principles

1. **Open-source** — always free and transparent.
2. **Zero config by default** — `pico build` just works. No CMakeLists.txt, no Makefile.
3. **Approachable and casual** — easy for a beginner to use *and* understand the codebase.
4. **Incremental by design** — only rebuild changed files and their transitive dependents,
   using content hashes (Blake3) combined with compiler-generated dependency (`.d`) files,
   not timestamps.
5. **Clear diagnostics** — meaningful output, verbose when asked, silent otherwise.
6. **Extensible** — easy config (`pico.toml`), a real plugin system, and debugging (`pico explain`).
7. **Interoperable, not imperialist** — pico should be able to *sit on top of* an existing
   CMake/Premake project via import, not require the user to throw their build system away
   on day one.

## Non-goals (for now)

Explicitly out of scope until later phases, so scope stays honest:

- Writing a CMake-language interpreter. Pico will never parse `CMakeLists.txt` directly —
  see "Interop & Plugin Architecture" below.
- Building and hosting pico's own package registry / binary cache. Package *sourcing* is
  delegated to vcpkg (see "Package Management").
- Full cross-platform support before Phase 4. Linux/macOS via GCC/Clang first; MSVC support
  is a distinct, scoped effort (see Toolchain Abstraction).
- Remote/distributed build caching (sccache-style) — Phase 4+ at the earliest.

## Language & Build

- **Language:** C++20
- **Build system:** CMake (dogfooding will come once pico can build itself)
- **Dependencies:** Blake3 (git submodule, C library for fast hashing)
- **Config parsing:** Custom TOML lexer/parser written in C (in `src/utils/toml_parser/`)
- **Package sourcing:** vcpkg (manifest mode), invoked as a subprocess — see below.

## Project structure

```
├── CMakeLists.txt
├── ROADMAP.md
├── design-doc.md
├── list_of_thought_commands.md
├── README.md
├── CONTRIBUTING.md
├── src/
│   ├── main.cpp
│   ├── cli/                    # CLI dispatch and argument parsing
│   ├── init/                   # `pico init` — project scaffolding
│   ├── scanner/                # Recursive file walk + hashing
│   ├── utils/
│   │   ├── hasher.hpp/.cpp     # Blake3 wrapper
│   │   └── toml_parser/        # Full TOML lexer + parser (C)
│   ├── diagnostics/            # Event counters and output
│   ├── cache/                  # Persistent build cache (hash + .d-derived edges)
│   ├── config/                 # pico.toml config reader
│   ├── compiler/                # Toolchain abstraction + compiler invocation
│   ├── graph/                  # .d-file parsing, include DAG construction
│   ├── scheduler/              # Topological sort, parallel build scheduling
│   ├── linker/                 # Object file linking
│   ├── runner/                 # Binary execution
│   ├── doctor/                 # Environment diagnostics
│   ├── fmt/                    # Code formatting
│   ├── pkg/                    # vcpkg subprocess integration (Phase 2)
│   └── plugins/                # Plugin discovery, manifest parsing, subprocess protocol (Phase 4)
├── tests/                       # Test suite — get this populated early, not deferred
├── benchmarks/
└── docs/
```

## Canonical Project Model (the pico IR)

Every part of pico's build pipeline (`graph`, `scheduler`, `compiler`, `linker`) consumes
**one internal representation**, regardless of whether the project came from `pico.toml`,
a CMake import, a Premake import, or a `compile_commands.json` import. This is the same
principle compilers use internally (front ends lower to one IR; back ends only ever see
the IR) — it means importers are the *only* code that needs to know anything about a
foreign build system.

```cpp
struct PicoTarget {
    std::string name;
    enum class Kind { Executable, StaticLib, SharedLib } kind;
    std::vector<std::string> sources;
    std::vector<std::string> include_dirs;
    std::vector<std::string> defines;
    std::vector<std::string> compile_flags;
    std::vector<std::string> link_libs;      // external libs (from vcpkg or system)
    std::vector<std::string> link_targets;   // other PicoTargets in this project
};

struct PicoProject {
    std::vector<PicoTarget> targets;
};
```

`config/` builds this from `pico.toml` directly. Importers (native or plugin) build it
from whatever foreign format they read. Nothing downstream cares which path it came from.

## Build lifecycle (native `pico.toml` path)

```
pico build
    │
    ├─ main.cpp ──► cli/ ──► dispatch "build"
    │
    ├─ config/  ──► parse pico.toml ──► build PicoProject (IR)
    │
    ├─ scanner/ ──► recursively walk source dirs listed in IR, hash every file (Blake3)
    │
    ├─ cache/   ──► compare hashes against last-known-good cache; mark changed files
    │
    ├─ graph/   ──► read prior .d files (if present) for changed units; on cache miss
    │               with no .d file, mark unconditionally dirty; build the include DAG;
    │               mark transitive dependents of any changed node
    │
    ├─ scheduler ──► topological sort of the DAG; produce a parallel compile plan
    │
    ├─ compiler/ ──► invoke toolchain (clang++/g++, later MSVC) with -MMD -MF per unit;
    │                re-read the freshly emitted .d file, update graph/cache for next run
    │
    └─ linker/  ──► link compiled objects (+ vcpkg-provided libs) into final binary/library
```

## Toolchain abstraction

`compiler/` must not be an if/else on compiler name. Define a `Toolchain` interface early:

```cpp
class Toolchain {
public:
    virtual CompileResult compile(const CompileUnit&) = 0;
    virtual LinkResult    link(const LinkPlan&) = 0;
    virtual Diagnostics   parse_diagnostics(const std::string& raw_output) = 0;
};
```

GCC/Clang share near-identical CLI dialects and can likely share one implementation to
start. MSVC (`/`-style flags, different diagnostic format, different dependency-file
mechanism — `/sourceDependencies`) becomes an additive `MsvcToolchain` later without
touching `graph/`, `scheduler/`, or anything upstream of `compiler/`.

## CLI commands — detailed pipelines

### `pico init` (Phase 0)
1. Verify target directory is empty or `--force` is passed.
2. Run `git init` if not already a repo.
3. Create standard directories (`src/`, `tests/`, etc.) from a template.
4. Write `pico.toml` from `src/init/config.toml` template, filling in project name.
5. Write `.gitignore` from `src/init/pico_gitignore.txt`.
6. Print next-steps hint (`pico build`, `pico run`).

### `pico build` (Phase 1)
1. Load and validate `pico.toml` via `config/` → build `PicoProject` IR.
2. If plugin-imported (see Interop section), merge imported `PicoTarget`s instead.
3. `scanner/` walks all source dirs referenced by the IR, hashes every file.
4. `cache/` diffs hashes against the persisted cache (`.pico/cache.bin` or similar).
5. `graph/` combines prior `.d`-derived edges with the diff to compute the dirty set
   (changed files ∪ transitive dependents via include edges).
6. `scheduler/` topologically sorts the dirty set into a parallel compile plan
   (respecting `-j`).
7. `compiler/` spawns the toolchain per unit, N at a time; captures diagnostics;
   re-parses freshly written `.d` files back into `graph/` and `cache/`.
8. On any compile failure: stop scheduling new units (unless `--keep-going`), print
   diagnostics, exit nonzero.
9. `linker/` links surviving object files (+ resolved vcpkg libs) per target.
10. Print a build summary (files compiled, cache hits, wall time) via `diagnostics/`.

### `pico add <package>` (Phase 2)
1. Parse package name (+ optional version constraint).
2. Append to the internal `vcpkg.json` manifest pico manages in the project root.
3. Shell out to `vcpkg install` (manifest mode) for the resolved triplet.
4. Parse resulting `vcpkg_installed/<triplet>/include` and `/lib`.
5. Record resolved include/lib paths + version in `pico.lock`.
6. Update `pico.toml`'s `[dependencies]` table for human visibility.

### `pico remove <package>` (Phase 2)
1. Remove from `vcpkg.json` and `pico.toml`.
2. Re-run `vcpkg install` so the manifest and installed tree stay consistent.
3. Update `pico.lock`.

### `pico update` (Phase 2)
1. Re-resolve versions against the current `vcpkg-configuration.json` baseline
   (or a newer one if `--latest` is passed).
2. Re-run `vcpkg install`; diff resulting versions against `pico.lock`; report changes.

### `pico run` (Phase 3)
1. Invoke `pico build` internally (same pipeline, no duplicate logic).
2. On success, exec the resulting binary, passing through any args after `--`.

### `pico test` (Phase 3)
1. Discover test targets (convention: `tests/` dir, or `[[test]]` entries in `pico.toml`).
2. Build each as its own target via the standard build pipeline.
3. Run each, capture pass/fail + timing, print a summary table.
4. Nonzero exit if any test fails.

### `pico clean` (Phase 3)
1. Remove build output directory.
2. `--cache` flag additionally clears `cache/`'s persisted hash/graph state.

### `pico doctor` (Phase 3)
1. Check for a working `clang++`/`g++`/`cl.exe` on `PATH`; report version.
2. Check `vcpkg` is bootstrapped and reachable.
3. Validate `pico.toml` parses cleanly.
4. Check cache file integrity (not corrupted/truncated).
5. Print a pass/fail checklist.

### `pico fmt` (Phase 3)
1. Locate `.clang-format` (project's own, or pico's default).
2. Run `clang-format -i` across all tracked source files.

### `pico check` (Phase 3)
1. Same as `build` through the `compiler/` stage, but pass a syntax/semantic-only flag
   (`-fsyntax-only` for GCC/Clang) — no object files emitted, no linking.

### `pico explain` (Phase 3)
1. Re-run the dirty-set computation from `graph/`/`cache/` without compiling.
2. For each file, print why it's dirty: content-hash mismatch, or transitive dependent
   of file X via header Y, or "not dirty — cache hit."

### `pico graph` (Phase 3)
1. Emit the include DAG (from `graph/`) as Graphviz `.dot` (`--dot`) or a simple text tree.

### `pico import <cmake|premake|compile-commands>` (Phase 4)
1. Dispatch to the relevant importer (native, or a discovered plugin).
2. Importer produces a `PicoProject` IR (JSON on stdout, for subprocess plugins).
3. Pico validates the IR, then either builds directly from it or writes an equivalent
   `pico.toml` the user can commit and hand-edit going forward (`--emit-toml`).

## Interop & Plugin Architecture (Phase 4)

**Principle:** importers are the only code allowed to know about a foreign build system;
everything else only ever sees the `PicoProject` IR.

### First-party (native) importers
- **CMake:** never parse `CMakeLists.txt`. Configure the project normally
  (`cmake -S . -B <dir>` with a CMake File API query file present), then read the JSON
  reply from `.cmake-api/v1/reply/` (targets, sources, include dirs, compile defs, link
  deps) and map it onto `PicoTarget`. This is the same mechanism CLion/Visual Studio use.
- **compile_commands.json:** a generic fallback importer. CMake, Meson, and Bazel can
  emit this natively; Premake can via an export add-on. One importer here covers a lot
  of ground for very little bespoke code.
- **Premake:** ship a small `export_pico.lua` Premake "action" that walks Premake's own
  in-memory solution/project objects and serializes them as JSON, then invoke
  `premake5 --file=export_pico.lua export` and consume that JSON the same way.

### Third-party (user-authored) plugins
- Protocol: subprocess + JSON over stdio (same shape as LSP servers / git credential
  helpers). A plugin is any executable; pico calls it with a request payload on stdin
  or as `--project-dir <path>`, and expects a `PicoProject`-shaped JSON on stdout,
  exit code 0 on success.
- No `dlopen`/shared-library ABI plugins for now — C++ ABI isn't stable across
  compilers/STL implementations, and import only runs once per configure, so subprocess
  overhead is irrelevant.
- Discovery: `[plugins]` table in `pico.toml` (name → executable path), or a
  `.pico/plugins/` directory, each with a `plugin.toml` manifest (name, protocol
  version, plugin type: `importer` / `pre-build-hook` / `post-build-hook` / `formatter`).
- Protocol is versioned from day one (`PICO_PLUGIN_PROTOCOL_VERSION`) so the schema can
  evolve without silently breaking existing plugins.

## Package Management (Phase 2) — via vcpkg

Pico does **not** implement its own registry, resolver, or per-package build recipes.
vcpkg already solves package sourcing and building; pico's job is compiling and linking
the user's own code against what vcpkg produces.

- Pico manages a `vcpkg.json` manifest on the user's behalf (mirrored from `pico.toml`
  `[dependencies]` for human readability).
- `pico add/remove/update` shell out to `vcpkg install` in manifest mode and read the
  resulting `vcpkg_installed/<triplet>/include` and `/lib` directories directly —
  **no CMake toolchain-file integration required**, since manifest-mode install alone
  populates that directory regardless of the consuming build system.
- Triplet selection (`x64-linux`, `x64-windows`, static/dynamic) is derived from pico's
  own build config (arch, `--release`, etc.) and passed to `vcpkg install --triplet ...`.
- Reproducibility comes from vcpkg's own versioning (`builtin-baseline` in
  `vcpkg-configuration.json`); `pico.lock` mirrors the resolved versions vcpkg reports,
  primarily so pico can skip re-invoking vcpkg when nothing changed.

## Module responsibilities

| Module         | Status        | Responsibility                                          |
| -------------- | ------------- | --------------------------------------------------------- |
| `cli/`         | ✅ Partial     | Parse CLI args, dispatch to the right command handler     |
| `init/`        | ✅ Partial     | `pico init` — scaffold a new project                      |
| `utils/`       | ✅ Mostly done | Blake3 hasher, TOML parser, file system helpers            |
| `scanner/`     | ✅ Partial     | Walk directory tree, hash source files                     |
| `diagnostics/` | ✅ Partial     | Event counters, summary printing                            |
| `cache/`       | ❌ TBD         | Persistent hash map + `.d`-derived edge cache               |
| `config/`      | ❌ TBD         | Read `pico.toml`, build `PicoProject` IR                    |
| `graph/`       | ❌ TBD         | Parse `.d` files, build/maintain the include DAG            |
| `scheduler/`   | ❌ TBD         | Topological sort, thread pool management                    |
| `compiler/`    | ❌ TBD         | `Toolchain` interface + GCC/Clang implementation             |
| `linker/`      | ❌ TBD         | Link object files into executables / libraries               |
| `runner/`      | ❌ TBD         | Build and run executables with arg passthrough                |
| `doctor/`      | ❌ TBD         | Verify toolchain and environment                              |
| `fmt/`         | ❌ TBD         | Run code formatter across the project                          |
| `pkg/`         | ❌ TBD         | vcpkg subprocess integration, manifest management               |
| `plugins/`     | ❌ TBD         | Plugin discovery, manifest parsing, subprocess protocol           |

See [ROADMAP.md](https://github.com/ka1rav6/pico/blob/main/ROADMAP.md) for the full
phased development plan with granular steps.
