# @partoska/bikini

Bikini — query and edit INI files from the command line. Shell scripts and
pipelines read and change configuration without `sed` or `awk`.

[![npm version](https://img.shields.io/npm/v/@partoska/bikini.svg)](https://www.npmjs.com/package/@partoska/bikini)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](https://github.com/partoska/bikini/blob/main/LICENSE.txt)

## What is this?

`bikini` reads INI files and prints what a query selects: the whole document,
one section, or one value.

**Key features:**

- Simple queries: `.`, `SECTION`, `SECTION.KEY`, with quoting for names that
  contain dots.
- Lists section or key names, one per line, for shell loops.
- Merges several files in order, later values replacing earlier ones.
- Sets and deletes keys and sections, printing the result or writing it back
  in place.
- Meaningful exit codes: `0` found, `1` not found, `2` usage or input error.
- Output of sections and documents is valid INI, so it pipes into another
  `bikini`.
- Works on macOS, Linux (x64, arm64) and Windows (x64, arm64).

## Install

```bash
npm install -g @partoska/bikini
```

This installs a small launcher plus the prebuilt native binary for your
platform, selected automatically. No compiler or build tools are required — see
[How this package works](#how-this-package-works) for the details.

### Supported platforms

| OS      | Architectures              |
| ------- | -------------------------- |
| macOS   | Intel & Apple Silicon      |
| Linux   | x64, arm64                 |
| Windows | x64, arm64                 |

## Quick Start

```bash
bikini server.port app.ini                # Print one value.
bikini server app.ini                     # Print one section, as INI text.
bikini -k . app.ini                       # List the sections.
cat app.ini | bikini server.host          # Read from standard input.
bikini server.port defaults.ini local.ini # Merge files, later ones win.
bikini -w -s 9090 server.port app.ini     # Set a value in place.
port=$(bikini server.port app.ini) || echo "no port"
```

Run `bikini -h` at any time for full usage.

## Usage

```bash
bikini [OPTIONS] [QUERY] [FILE...]
```

Reads `FILE`, or standard input when `FILE` is missing or `-`.

| Query         | Prints                                 |
| ------------- | -------------------------------------- |
| `.` (default) | the whole document, normalized         |
| `SECTION`     | one section, as INI text               |
| `SECTION.KEY` | the raw value of a key                 |

A whole section name wins over a split at the dot, and otherwise the key
follows the last dot, so `database.primary.max-connections` reads
`max-connections` from `[database.primary]`. Names with dots can be quoted:
`'"a.b"."c.d"'` reads key `c.d` of section `[a.b]`; within quotes `\"` and
`\\` escape.

| Flag | Long            | Meaning                                                |
| ---- | --------------- | ------------------------------------------------------ |
| `-k` | `--keys`        | List the sections of `.`, or the keys of `SECTION`.    |
| `-s` | `--set VALUE`   | Set `SECTION.KEY` to `VALUE` and print the document.   |
| `-d` | `--delete`      | Delete `SECTION` or `SECTION.KEY`, print the document. |
| `-w` | `--write`       | Save the edit back to the single `FILE` instead.       |
| `-i` | `--ignore-case` | Match sections and keys case-insensitively.            |
| `-h` | `--help`        | Show the help.                                         |
| `-V` | `--version`     | Show the version.                                      |

Exit status: `0` found, `1` not found, `2` usage or input error (including
syntax errors in the INI text, reported with their line on standard error).

## How this package works

`@partoska/bikini` follows the per-platform `optionalDependencies` model. The
package you install ships only a tiny Node launcher; the actual native binary
comes from one of these platform packages, and npm installs only the one
matching your OS and CPU:

- `@partoska/bikini-darwin` — macOS universal (Intel + Apple Silicon)
- `@partoska/bikini-linux-x64`
- `@partoska/bikini-linux-arm64`
- `@partoska/bikini-win32-x64`
- `@partoska/bikini-win32-arm64`

When you run `bikini`, the launcher runs the matching binary with the standard
streams passed through and exits with its status, so pipes, redirections and
exit codes behave exactly as with the binary itself.

## Troubleshooting

### "Unsupported platform" or "could not find the native binary"

This usually means optional dependencies were skipped during install (for
example with `npm install --omit=optional`, or behind a strict CI mirror).
Reinstall with optional dependencies enabled:

```bash
npm install -g @partoska/bikini
```

## License

MIT. Copyright (C) 2026 Fabrika Charvat s.r.o. Developed by the
[Partoska Laboratory](https://lab.partoska.com) team.

## Links

- **Project website:** <https://lab.partoska.com/bikini>
- **Source & issues:** <https://github.com/partoska/bikini>
