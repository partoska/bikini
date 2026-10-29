# Contributing to bikini

Thank you for your interest in contributing! Here's everything you need to get started.

## Getting Started

### Prerequisites

- C99-capable compiler (GCC, Clang, or MSVC on Windows)
- CMake 3.20+ (3.23+ to package for macOS)
- Git
- On Windows: Git Bash or MSYS2 to run the scripts and the CLI tests

### Building

```bash
git clone https://github.com/partoska/bikini.git
cd bikini

# Debug build with sanitizers and tests (recommended for development)
./build.sh -d -s -t

# The tool will be at: build/src/cli/bikini (build/bin/bikini.exe on Windows)
```

### Build options

| Flag | Description |
|------|-------------|
| `-d` | Debug build (includes debug symbols) |
| `-s` | Enable address and undefined behavior sanitizers (GCC/Clang) |
| `-t` | Build and register unit tests |
| `-v` | Verbose debug logging (`DEBUG_SLOW` macro) |
| `-j N` | Use N parallel build jobs |
| `-p` | Create the installation packages in `dist/` |

### Running tests

```bash
./build.sh -d -s -t
./test.sh
```

`test_bikini` covers the library, `test_cli.sh` the `bikini` tool end to end.
Without CMake, `make test` builds and runs both as well.

## Submitting Changes

1. Fork the repository and create a branch from `main`.
2. Make your changes — keep commits focused and atomic.
3. Run a debug build with sanitizers and tests (`./build.sh -d -s -t`) and verify it builds cleanly.
4. Run the tests (`./test.sh`) and verify they all pass.
5. Format your code with `clang-format` (configuration is in `.clang-format`).
6. Open a pull request with a clear description of what you changed and why.

## Code Style

- **Standard**: ISO C99 (`-std=c99 -pedantic`), no compiler extensions.
- **Formatter**: `clang-format` (GNU style — run it before committing); braces on every `if`/`else`/loop body.
- **Naming**:
  - Types: `CamelCase` with the `BK` prefix — the C types too (`BKSize`, `BKChar`, `BKInt`); unsigned char, short and int are `BKByte`, `BKWord`, `BKDword`.
  - Functions: `camelCase` with `bk` prefix (e.g., `bkParse`, `bkSectionFind`), `bkc` (bikini CLI) in the command line tool; its types and macros use `BKC`/`BKC_`.
  - Macros: `UPPER_SNAKE_CASE` with `BK_` prefix.
- **Portability**: system headers and C library calls only in the `plat.{h,c}` layers.
- **Warnings**: All warnings are treated as errors (`-Wall -Wextra -Werror`, `/W3 /WX` on MSVC) — fix them, don't suppress them.
- **Library rules**: errors are recorded in the document (`bkError()`), the library never prints and has no global state.

## Adding a Source File

The CMake build and the hand-written `Makefile` must stay in sync. When adding a
source file, list it in both:

1. `src/lib/CMakeLists.txt` or `src/cli/CMakeLists.txt`
2. `Makefile` — `LIB_SRCS` or `CLI_SRCS`

## Changing the Command Line

When adding or changing a command-line option or the query language, also update:

1. `src/cli/src/main.c` — the built-in help (`bikini -h`)
2. `test/test_cli.sh` — the end-to-end tests
3. `README.md` — the command line section
4. `npm/bikini/README.md` — the usage tables
5. `assets/installer/README.*.txt` — the quick start of the installers
6. `skills/bikini/SKILL.md` — the agent skill

## Reporting Issues

Please use the GitHub issue tracker. Bug reports are most useful when they include:

- The version of `bikini` (`bikini --version`)
- Your operating system and architecture
- The exact command you ran and its exit status
- The smallest INI input that shows the problem
- The full output, including any error messages

## License

By contributing, you agree that your contributions will be licensed under the [MIT License](LICENSE.txt).
