# Design Doc

## What does pico aim to solve?
Pico aims to be an extremely fast cargo-like build system and (in the future) maybe even a package system for c++ files.
It will completely eliminate the need of `cmake` and `make` configurations
I want it to be simple, straightforward, easy to use and configure, and useful for anyone who uses it.

## Project structure (temp)

```tree
├── LICENSE
├── README.md
├── benchmarks/
├── build.zig
├── build.zig.zon
├── design-doc.md
├── docs/
├── list_of_thought_commands.md/
├── src/
│   ├── cache/
│   ├── cli/
│   ├── compiler/
│   ├── config/
│   ├── diagnostics/
│   ├── doctor/
│   ├── fmt/
│   ├── graph/
│   ├── linker/
│   ├── root.zig
│   ├── runner/
│   ├── scanner/
│   ├── scheduler/
│   └── utils/
├── tests
└── zig-out
```

I am trying to make every folder clearly show what it contains and be very easy for a beginner to understand.


## Core principles of Pico:
1. Open-source
2. Zero config by default
3. Easy for a beginner to not only use, but also understand the codebase
4. Fast incremental builds of only changed files + dependent ones
5. Clear diagnostics and good verbose compilation
6. Easy config + debugging



## Lifecycle of build:

Start : `pico build` -> `main.zig` hands off command to `cli/`
cli-> executes build fn defined in `src/build.zig`
build function does the following :
check using `scanner` and `cache` if compiling is needed.
the cache is updated too
marks file as changed and uses `graph` to find other files that require compilation
forwards to `compiler` to compile the new files in a threaded manner (order decided using `scheduler`)
then these all get linked together using `linker`

