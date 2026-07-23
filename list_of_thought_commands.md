# Pico CLI

## 1. `pico init [directory]`
Initializes a new Pico project.

### What it does
- Creates the project structure.
- Creates `src/`, `include/`, `tests/`, `build/`, and `libs/`.
- Creates `pico.toml`.
- Creates or updates `.gitignore` to ignore `build/`.
- Verifies that a supported compiler is installed.
- Creates a simple `main.cpp`.
- Attempts a test build.

### Flags
- `--git` - Initialize a Git repository.
- `--clang` - Use Clang as the default compiler.
- `--gcc` - Use GCC as the default compiler.

---

## 2. `pico build`
Builds the project.

### What it does
- Scans the project.
- Detects changed files.
- Builds only what is necessary.
- Links the executable(s).

### Flags
- `--release`
- `--clean` (ignore cache and rebuild everything)
- `--verbose`
- `-j <threads>` (number of compile threads)

---

## 3. `pico run`
Builds (if required) and runs an executable.

### What it does
- Builds only if needed.
- Runs the selected executable.
- Passes arguments to the executable.

### Flags
- `--release`
- `-- <args...>` (arguments passed to the executable)

### Options
- `<target>` (run a specific executable if multiple exist)

Examples
```bash
```
```bash
pico run

pico run editor

pico run -- input.txt

pico run game -- level1 save.dat
```
```
```
---

## 4. `pico test`
Builds and runs all tests.

### What it does
- Finds test files.
- Builds the test executable(s).
- Runs every test.
- Prints a test summary.

### Flags
- `--release`
- `--verbose`

---

## 5. `pico clean`
Removes generated build files.

### What it does
- Deletes `build/`.
- Clears intermediate object files.
- Optionally clears the build cache.

### Flags
- `--cache`

---

## 6. `pico add <package>`
Adds a dependency to the project.

### What it does
- Downloads the package.
- Stores it inside `libs/` or the Pico cache.
- Updates `pico.toml`.
- Makes the package available during builds.

Example

```bash
pico add fmt
pico add glfw
```

---

## 7. `pico remove <package>`
Removes a dependency.

### What it does
- Removes it from the project configuration.
- Cleans unused cached files.

---

## 8. `pico update`
Updates project dependencies.

### What it does
- Checks for newer package versions.
- Updates the dependency lock file.
- Rebuilds if required.

---

## 9. `pico explain`
Explains what happened during the previous build.

### What it does
Shows:
- Which files were rebuilt.
- Which files were skipped.
- Why each file was rebuilt.
- Cache hits/misses.
- Total build timings.

---

## 10. `pico graph`
Displays the project's dependency graph.

### What it does
- Shows source/header dependencies.
- Helps visualize project structure.

### Flags
- `--dot` (export Graphviz format)

---

## 11. `pico doctor`
Checks whether the development environment is correctly configured.

### What it does
Checks:
- Compiler
- Linker
- Debugger
- Project configuration
- Cache integrity
- Missing dependencies

---

## 12. `pico fmt`
Formats the project's source code.

### What it does
- Runs the configured formatter (e.g. clang-format).

---

## 13. `pico check`
Performs static analysis without producing executables.

### What it does
- Runs compiler diagnostics.
- Performs static analysis.
- Reports warnings and errors quickly.

---

# Configuration

Project settings are stored inside `pico.toml`.

Example:
```toml
```
```toml
name = "my-project"

[build]
compiler = "clang++"
cpp_standard = "23"
optimization = "debug"
warnings = true
shared = false
threads = 8

[dependencies]
fmt = "latest"
glfw = "3.4"
```
```
```
