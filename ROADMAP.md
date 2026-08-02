# Pico Roadmap

> **Current phase: Phase 0 — Scaffolding ~80% complete**

This roadmap breaks every phase down into the actual sub-steps needed to
finish it, not just a checklist of features. Each command has its own
"deep dive" describing exactly what has to happen internally, in order,
before that command can be considered done.

---

## Phase 0: Foundation (Scaffolding)

- [x] Project pivot from Zig to C++
- [x] CMake build system (C++20)
- [x] Blake3 integration (submodule)
- [x] CLI dispatcher (`main.cpp` → `cli_entry.cpp`)
- [x] `pico init` — project scaffolding (git init, dirs, config.toml, .gitignore)
- [x] Scanner — recursive file walk + Blake3 hashing
- [x] Hasher — Blake3-based file hashing utilities
- [x] TOML lexer/parser (full implementation)
- [x] Diagnostics — event counters and output printing
- [ ] CLI argument parsing (flags, options, subcommands)
  - [ ] Define a minimal `ArgSpec` per command: positional args, flags (bool),
        options (string/int with value), and a `--help` auto-generator
  - [ ] Support `--flag=value` and `--flag value` forms
  - [ ] Support short flags (`-j4`) aliasing long ones (`--jobs=4`)
  - [ ] Unknown-flag error path: suggest the closest known flag (edit distance)
  - [ ] Unit tests: one test file per command's arg spec, feeding malformed
        input and asserting on the exact error message
- [ ] Error handling infrastructure
  - [ ] Decide error model: exceptions vs a `Result<T, Error>`-style return
        type. Given "clear diagnostics" is a core principle, a `Result` type
        with structured `Error{code, message, hint}` is more consistent with
        `pico explain`/`pico doctor` later needing structured errors, not just
        strings — recommend this over exceptions.
  - [ ] Central `Error` enum covering: `IoError`, `ConfigParseError`,
        `CompilerNotFound`, `CircularDependency`, `LinkError`, `PluginError`
  - [ ] Every error carries enough context to print a one-line summary *and*
        a `--verbose` multi-line trace, without needing two error types
- [ ] Logging / verbose output infrastructure
  - [ ] Central `Logger` singleton or passed-context object (avoid a raw
        global if you want this testable later — inject it)
  - [ ] Levels: `quiet`, `normal`, `verbose`, `debug`
  - [ ] Structured events (reuses `diagnostics/event.hpp`) so `pico explain`
        can later replay *why* something happened, not just print it once

---

## Phase 1: Core Build Pipeline

**Goal:** `pico build` actually compiles and links a project, correctly,
incrementally, and in parallel.

### Step order matters here — build these in this sequence, not roadmap order:

1. **`config/`** first — nothing else can be tested without a real config
2. **`cache/`** — needed before `graph/` so graph work can be tested against
   real cache state instead of mocks
3. **`graph/`** — needed before `scheduler/`
4. **`scheduler/`** — needed before `compiler/` invocation is wired up for real
5. **`compiler/`** then **`linker/`**
6. **Wire `pico build`** end to end last, once every stage works standalone

### `config/` — read and validate `pico.toml`

- [ ] Define the config schema in code (a `PicoConfig` struct) before writing
      the reader — this becomes the contract for both hand-written and
      plugin-generated `pico.toml` files
- [ ] Required fields: project name, C++ standard, target type
      (executable/static_lib/shared_lib), source globs
- [ ] Optional fields: compiler override, extra flags, defines, include dirs,
      per-profile settings (`[profile.release]`, `[profile.debug]`)
- [ ] Validation pass: catch bad std versions, conflicting target types,
      glob patterns that match zero files — fail fast with a specific message
- [ ] Default values for every optional field, documented inline in the
      generated `config.toml` template from `pico init`

### `cache/` — persistent hash → status map

- [ ] On-disk format: a simple flat file (e.g. `.pico/cache.bin` or a small
      TOML/JSON) mapping `file path → {content_hash, mtime, last_compiled}`
- [ ] Load cache at build start; if missing or schema version mismatch,
      treat as a full rebuild (never crash on a stale cache)
- [ ] After a successful build, atomically write the updated cache
      (write to temp file, rename — avoid partial writes on crash/Ctrl-C)
- [ ] Decide invalidation trigger: content hash only, or hash + mtime as a
      fast-path pre-check before hashing (mtime unchanged → skip hash).
      The mtime pre-check matters a lot for build speed on large projects.

### `graph/` — `#include` dependency DAG

- [ ] Use the compiler's own `-MMD -MP -MF <file>.d` (GCC/Clang) output as
      the source of truth, per your decision — don't hand-parse includes
- [ ] Write a `.d` file parser: it's Makefile syntax but restricted to
      `target: dep1 dep2 \` continuation lines — handle escaped spaces in
      paths (common on Windows / paths with spaces)
- [ ] MSVC path: parse `/sourceDependencies <file>.json` output (structured
      JSON, actually easier than GCC's `.d` format) — implement as a second
      backend behind the same internal interface, selected by detected
      compiler, mirroring Ninja's `deps = gcc` / `deps = msvc` split
- [ ] Build the DAG: `changed file → set of dependents` (reverse edges),
      since what you need per-build is "given these changed files, what
      else must rebuild" — not the forward graph
- [ ] Handle the "no `.d` file exists yet" case explicitly: first-ever build
      of a file has nothing to parse, so it's unconditionally compiled and
      its `.d` is generated *during* that compile for next time
- [ ] Cycle detection: technically shouldn't happen with real includes, but
      guard against it and error clearly rather than infinite-looping

### `scheduler/` — topological sort + parallel plan

- [ ] Topological sort of the DAG restricted to "files that need rebuilding"
- [ ] Thread pool sized by `-j<N>` flag, default to hardware concurrency
- [ ] Work-stealing or a simple shared queue — for a first version, a plain
      mutex-guarded queue is fine; don't over-engineer this before it's a
      measured bottleneck
- [ ] Failure policy: if one compile fails, do in-flight compiles finish
      or abort immediately? (Cargo/Ninja let in-flight finish, then stop
      scheduling new work — recommend matching that, it avoids wasting
      already-started work)

### `compiler/` — invoke the toolchain

- [ ] Design a `Toolchain` interface now, even though only GCC/Clang ship
      first — this is where "MSVC later" either stays cheap or becomes a
      rewrite:
      ```
      struct Toolchain {
        virtual CompileResult compile(CompileUnit) = 0;
        virtual LinkResult link(std::vector<ObjectFile>, LinkSpec) = 0;
        virtual std::string flag_for_std(CppStandard) = 0;
        virtual std::string flag_for_include(path) = 0;
        virtual std::string flag_for_define(string) = 0;
        virtual DepFileFormat dep_file_format() = 0;
      };
      ```
- [ ] `GccClangToolchain` implementation first (they're close enough to
      share ~90% of flag logic)
- [ ] Capture stdout/stderr per compile unit, tag with the source file, so
      parallel output doesn't interleave into garbage — buffer per-job,
      flush in submission order or clearly labeled
- [ ] Parse compiler diagnostics into a structured form (file, line, col,
      severity, message) — needed later for `pico check` and IDE integration,
      cheap to do now while you're already touching this code
- [ ] Non-zero exit handling: report which file failed, keep the rest of the
      diagnostics output visible instead of swallowing it

### `linker/`

- [ ] Executable target: link all objects + resolved library flags
- [ ] Static library target: invoke `ar`/`llvm-ar` (not the linker directly)
- [ ] Shared library target: correct `-shared`/`-fPIC` handling per platform
- [ ] Library resolution order matters on Linux (link order affects symbol
      resolution with static libs) — preserve declaration order from config
      rather than reordering for "cleanliness"

### Wiring `pico build`

- [ ] `scanner → cache → graph → scheduler → compiler → linker`, each stage
      behind an interface so it's independently testable
- [ ] `--release` — switch flag/optimization profile from config
- [ ] `--clean` — wipe cache + build dir before building (not just skip cache)
- [ ] `--verbose` — full compiler command lines + timing per stage
- [ ] `-j<N>` — thread count override

---

## Phase 2: Package Management (via delegated plugins)

**Goal:** `pico add fmt` makes a library available, without pico owning
package resolution or hosting itself — see `PLUGINS.md` for the full design.

- [ ] Define the **Package Source Plugin** contract (`resolve`, `fetch`,
      `paths` verbs) — same JSON-over-stdio protocol as import plugins
- [ ] `pico-plugin-vcpkg` — first reference implementation
  - [ ] `resolve`: check if package+version exists in vcpkg's registry,
        return canonical name/version
  - [ ] `fetch`: run `vcpkg install <pkg>` for the detected/configured triplet
  - [ ] `paths`: return include dir, lib dir, and library names from the
        vcpkg installed tree so pico's own `compiler/`/`linker/` can consume
        them as ordinary flags — pico never needs to understand vcpkg's
        internal layout beyond this one call
- [ ] `pico add <package>` — invoke plugin `resolve` + `fetch`, write result
      into `[dependencies]` in `pico.toml`
- [ ] `pico remove <package>` — remove from config; leave the vcpkg cache
      alone by default (it's shared across projects) unless `--purge`
- [ ] `pico update` — re-run `resolve` for every dependency, diff against
      lockfile, report what would change before writing
- [ ] `pico.lock` — record resolved exact versions + which plugin resolved
      them, so a fresh clone reproduces the same build without re-resolving
- [ ] Document clearly: pico is not responsible for vcpkg's own ABI/triplet
      correctness (debug/release CRT mismatches etc.) — surfaced, not solved

---

## Phase 3: Developer Experience

Each of these should get its own short design note before implementation —
they're small individually but easy to under-scope.

- [ ] **`pico run`** — build (respecting cache), then exec the resulting
      binary with `--` passthrough for its own argv; propagate its exit code
- [ ] **`pico test`** — needs a test-discovery convention first (e.g. any
      target tagged `[test]` in `pico.toml`, or files under `tests/`);
      build all test targets, run each, aggregate pass/fail into a summary
      table, non-zero exit if any failed
- [ ] **`pico clean`** — remove build artifacts; `--cache` flag to also wipe
      the incremental cache (separate from artifact cleanup, since users
      often want one without the other)
- [ ] **`pico doctor`** — checks in order: compiler found + version, linker
      found, cache directory writable, `pico.toml` parses, (later) each
      declared plugin is present on PATH and responds to a handshake
- [ ] **`pico fmt`** — shell out to `clang-format` with a project-local
      `.clang-format` if present, else a bundled default; `--check` mode
      that fails without modifying files, for CI use
- [ ] **`pico check`** — compile with `-fsyntax-only` (or equivalent) to get
      diagnostics without producing binaries; reuse the same diagnostic
      parser built for `compiler/`
- [ ] **`pico explain`** — replay the structured events from `diagnostics/`:
      for the last build, show per-file cache hit/miss, why each file was
      marked dirty (content changed vs. a dependency changed vs. flags
      changed), and per-stage timing
- [ ] **`pico graph`** — visualize the `#include` DAG; `--dot` export for
      Graphviz; consider a `--focus <file>` mode that only shows the subgraph
      relevant to one file, since full-project graphs get unreadable fast

---

## Phase 4: Production Readiness

- [ ] Cross-platform support (Linux, macOS, Windows) — this is where the
      `Toolchain` abstraction from Phase 1 either pays off cheaply or costs
      a rewrite; add the MSVC backend here
- [ ] Comprehensive test suite (should already exist incrementally from
      Phase 1 onward — this phase is about coverage gaps, not starting from
      zero)
- [ ] Benchmarks vs CMake + Make / Ninja, on a real third-party codebase,
      not just pico building itself
- [ ] Continuous integration (GitHub Actions) across all three platforms
- [ ] Precompiled headers (PCH) support
- [ ] Distributed / remote build caching (sccache-style)
- [ ] IDE integration (`compile_commands.json` export, LSP support)
- [ ] Plugin registry / discovery beyond local PATH scanning
- [ ] Community documentation and contribution guides
- [ ] Releases and binary distribution

---

## Explicit non-goals (for now)

Naming these prevents scope creep from being accidental:

- Hosting pico's own package registry/index — delegate to vcpkg/Conan instead
- Hand-parsing CMake or Premake DSLs — always drive their own introspection APIs
- In-process (dlopen) plugins — process-boundary plugins only, until a
  measured performance need proves otherwise
- Full CMake feature parity — import covers targets/sources/flags/links,
  not arbitrary custom CMake script logic
