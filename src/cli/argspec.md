# ArgSpec

Reference for the CLI argument contract. Every command defines an `ArgSpec`:

| Kind | Meaning | Example |
| ---- | ------- | ------- |
| `positional` | Ordered, un-dashed argument | `pico add <package>` |
| `flag` | Boolean switch, no value | `--release` |
| `option` | Takes a value (`string` or `int`) | `--jobs=4` |
| `passthrough` | Everything after `--`, forwarded verbatim | `pico run -- -x foo` |

## Parsing conventions

- `--flag` — boolean flag
- `--flag=value` and `--flag value` — both forms accepted for options
- Short flags alias long ones: `-j4` ≡ `--jobs=4`, `-j 4` ≡ `--jobs 4`
- `--` — everything after is passthrough, not parsed as pico args
- `--help` — auto-generated per command (all commands)
- Unknown flag -> error naming the closest known flag (edit distance)
- Positional args are optional unless marked `<required>`

---

## pico init

Positional Arg : `[directory]`

Flags:

```bash
--git     # initialize a git repository
--clang   # default compiler: clang
--gcc     # default compiler: gcc
```

## pico build

Flags:

```bash
--release    # switch to the release profile
--clean      # wipe cache + build dir before building
--verbose    # full compiler command lines + per-stage timing
--keep-going # don't abort after a failed compile; finish in-flight work
```

Options:

```bash
-j, --jobs <int> # compile thread count; default: hardware concurrency
```

## pico run

Positional Arg : `[target]`

Flags:

```bash
--release
```

Passthrough:

```bash
-- <args...> # argvs for the program
```

## pico test

Flags:

```bash
--release
--verbose
```

## pico clean

Flags:

```bash
--cache # also wipe the incremental build cache
```

## pico add

Positional Arg : `<package>` (required)

Options:

```bash
# optional version constraint, e.g. `pico add fmt@3.4`
```

## pico remove

Positional Arg : `<package>` (required)

## pico update

Flags:

```bash
--latest # re-resolve against the newest baseline, not the pinned one
```

## pico explain

No arguments.

## pico graph

Flags:

```bash
--dot # export Graphviz format
```

Options:

```bash
--focus <file> # only show the subgraph relevant to one file
```

## pico doctor

No arguments.

## pico fmt

Flags:

```bash
--check # fail without modifying files (CI use)
```

## pico check

No arguments.

## pico import

Positional Arg : `<cmake|premake|compile-commands>` (required)

Flags:

```bash
--emit-toml # write an equivalent pico.toml instead of building directly
```

## pico help

Positional Arg : `[command]`

No flags.
