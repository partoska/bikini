---
name: bikini
description: Use the bikini CLI to read, list, merge, set, and delete values in INI configuration files from shell scripts and pipelines. Use when the user wants to query or edit INI, .cfg, or .conf files with sections and key = value lines, or script against them.
license: MIT
metadata:
  author: Partoska Laboratory
  version: "1.0.0"
---

# Bikini (bikini)

Bikini queries and edits INI files from the command line. It reads the common INI dialect (without interpolation) and prints exactly what a query selects, so shell scripts can read configuration without `sed` or `awk`.

## Mental model

- A **query** selects the whole document (`.`), one **section** (`SECTION`), or one **value** (`SECTION.KEY`). The default query is `.`.
- **Values** print raw: no quotes, no `key =`, one trailing newline. A multi-line value prints its lines joined with newlines.
- **Sections and documents** print as valid, normalized INI, so the output can be piped into another `bikini`.
- **Input** is every `FILE` given, or standard input when there is none or the file is `-`. Several files merge in order: later values replace earlier ones, a section repeated in a later file adds its keys.
- **Exit status** carries the answer: `0` found, `1` not found, `2` usage or input error. Errors go to standard error, never to standard output.
- **Edits** (`-s`, `-d`) print the whole edited document, or write it back to its single file with `-w`.

## Installation

Bikini is available for macOS, Linux, and Windows. If the environment cannot run installers, portable binaries or building from source are the fallbacks.

### Step 1: Install bikini

#### Option A: Package manager (recommended)

Homebrew on macOS or Linux (x86_64 / arm64):

```bash
brew install partoska/tap/bikini
```

Scoop on Windows (x64 / arm64):

```powershell
scoop bucket add partoska https://github.com/partoska/scoop-bucket
scoop install partoska/bikini
```

npm on macOS, Linux (x64, arm64), or Windows (x64, arm64):

```bash
npm install -g @partoska/bikini
```

Prefer npm when Node.js is already available; prefer Homebrew or Scoop when it is already the system's package manager.

#### Option B: Platform-specific binary from GitHub Releases

Download the latest release from GitHub:

**https://github.com/partoska/bikini/releases**

If network access is available and you want exact commands, resolve the latest release via the GitHub Releases API rather than inventing versioned URLs:

```bash
curl -L https://api.github.com/repos/partoska/bikini/releases/latest
```

Select the asset from `assets[].name`, use its `browser_download_url`, and verify against its `digest` when available. Do **not** hard-code a version or construct a release URL by guessing.

#### Choose the right asset

| Platform | Preferred interactive install | Portable/headless fallback |
|---|---|---|
| macOS | `bikini_<version>_darwin_universal.pkg` | `bikini_<version>_darwin_universal.tar.gz` or bare `bikini_<version>_darwin_universal` |
| Debian/Ubuntu amd64 | `bikini_<version>_linux_amd64.deb` | `bikini_<version>_linux_amd64.tar.gz` or bare `bikini_<version>_linux_amd64` |
| Debian/Ubuntu arm64 | `bikini_<version>_linux_arm64.deb` | `bikini_<version>_linux_arm64.tar.gz` or bare `bikini_<version>_linux_arm64` |
| Debian/Ubuntu armhf | `bikini_<version>_linux_armhf.deb` | `bikini_<version>_linux_armhf.tar.gz` or bare `bikini_<version>_linux_armhf` |
| Fedora/RHEL amd64 | `bikini_<version>_linux_amd64.rpm` | `bikini_<version>_linux_amd64.tar.gz` or bare `bikini_<version>_linux_amd64` |
| Fedora/RHEL arm64 | `bikini_<version>_linux_arm64.rpm` | `bikini_<version>_linux_arm64.tar.gz` or bare `bikini_<version>_linux_arm64` |
| Windows x64 | `bikini_<version>_windows_amd64_setup.exe` or `.msi` | `bikini_<version>_windows_amd64.zip` or bare `bikini_<version>_windows_amd64.exe` |
| Windows arm64 | `bikini_<version>_windows_arm64_setup.exe` | `bikini_<version>_windows_arm64.zip` or bare `bikini_<version>_windows_arm64.exe` |

Use `uname -s` for OS and `uname -m` for CPU architecture. Map `x86_64` to `amd64`; map `aarch64` or `arm64` to `arm64`; map `armv6l` or `armv7l` to `armhf`. On Linux, inspect `/etc/os-release` to choose `.deb` for Debian/Ubuntu-family systems and `.rpm` for Fedora/RHEL-family systems. On Windows, `$env:PROCESSOR_ARCHITECTURE` is `AMD64` or `ARM64`.

#### Install examples

macOS package:

```bash
sudo installer -pkg ./bikini_<version>_darwin_universal.pkg -target /
```

Debian/Ubuntu:

```bash
sudo apt install ./bikini_<version>_linux_amd64.deb
```

Fedora/RHEL:

```bash
sudo dnf install ./bikini_<version>_linux_amd64.rpm
```

Portable tarball (Linux or macOS). The archive extracts into a `bikini_<version>_<platform>/` directory with the binary somewhere under it in `bin/`, so locate it instead of assuming a path:

```bash
tar -xzf ./bikini_<version>_<platform>.tar.gz
mkdir -p ~/.local/bin
install -m 755 "$(find ./bikini_<version>_<platform> -type f -path '*/bin/bikini')" ~/.local/bin/bikini
```

Portable bare binary:

```bash
chmod +x ./bikini_<version>_<platform>
mkdir -p ~/.local/bin
mv ./bikini_<version>_<platform> ~/.local/bin/bikini
```

Make sure `~/.local/bin` is on `PATH` when using portable installs. On Windows, put the bare `.exe` (renamed to `bikini.exe`) in a directory on `PATH`.

#### Verify downloaded assets

When the release API provides a `digest` such as `sha256:<hex>`, verify the downloaded file before installing:

```bash
shasum -a 256 ./bikini_<version>_<platform>.<ext>
```

Compare the output hash to the asset digest from the release API.

### Step 2: Verify the install

```bash
bikini --version
```

## Quick reference

```
bikini [-i] [-k | -s VALUE | -d] [-w] [QUERY] [FILE...]
bikini -h                                       # full help
bikini -V                                       # print version
```

### Queries

| Query | Prints |
|---|---|
| `.` (default) | The whole document, normalized. |
| `SECTION` | One section, as INI text (`[SECTION]` and its keys). |
| `SECTION.KEY` | The raw value of one key. |
| `'"a.b"."c.d"'` | Quoted names: key `c.d` of section `[a.b]`. |

How a query splits into section and key:

- **Unquoted**: a whole section name wins; otherwise the query splits at the **last** dot. `database.primary.max-connections` reads key `max-connections` from `[database.primary]`, and `database.primary` alone is the section `[database.primary]`.
- **Quoted**: as soon as the query contains a `"`, only dots outside of quotes split. The last name is the key, the others, joined by dots, the section. Inside quotes, `\"` and `\\` escape. Wrap the whole query in single quotes for the shell: `bikini 'logging."handler.file"' app.ini`.

### Options

| Flag | Long | Meaning |
|---|---|---|
| `-k` | `--keys` | List names, one per line: the sections of `.`, or the keys of `SECTION`. |
| `-s VALUE` | `--set VALUE` | Set `SECTION.KEY` to `VALUE` (adding section and key when missing), print the document. |
| `-d` | `--delete` | Delete `SECTION` or `SECTION.KEY`, print the document. |
| `-w` | `--write` | With `-s` or `-d`: write the document back to `FILE` instead of printing it. Needs exactly one `FILE`. |
| `-i` | `--ignore-case` | Match section and key names ASCII case-insensitively. |
| `-h` | `--help` | Show the help. |
| `-V` | `--version` | Show the version. |

`-k`, `-s`, and `-d` exclude each other. `-s` always splits its query into section and key (never a whole section).

### Exit codes

| Code | Meaning |
|---|---|
| `0` | Found (or edit done). |
| `1` | Not found: missing section or key, or a delete target that is not there. Nothing on standard error. |
| `2` | Usage or input error: bad option, unreadable file, INI syntax error (reported with its line), duplicate section or key within one file, `-k` on a value, `-w` without exactly one file. |

## Common workflows

Read one value, with a fallback when it is missing:

```bash
port=$(bikini server.port app.ini) || port=8080
```

Distinguish "missing" from "broken" in a script:

```bash
if value=$(bikini server.host app.ini); then
  echo "host is $value"
else
  case $? in
    1) echo "no server.host" ;;
    *) echo "cannot read app.ini" >&2; exit 2 ;;
  esac
fi
```

Loop over sections or keys:

```bash
for section in $(bikini -k . app.ini); do
  echo "[$section] has $(bikini -k "$section" app.ini | wc -l) keys"
done
```

Layer configuration, later files winning (defaults, then local overrides):

```bash
bikini server.port defaults.ini local.ini
bikini . defaults.ini local.ini > merged.ini
```

Edit in place, or write the edit elsewhere:

```bash
bikini -w -s 9090 server.port app.ini
bikini -d server.debug app.ini > clean.ini
bikini -d -w client app.ini || echo "no [client] section"
```

Pipelines (output of `.` and `SECTION` is valid INI):

```bash
curl -s https://example.com/app.ini | bikini server.host
bikini server app.ini | bikini -k
```

Multi-line values come back with their lines joined by newlines; set one with a value containing newlines:

```bash
bikini -s "$(printf 'db1.example.com\ndb2.example.com')" database.primary.hosts app.ini
```

## INI dialect

```ini
; comment            Comments take a whole line and start with ; or #.
[section]            Section names are case-sensitive by default (-i to ignore case).
key = value          Either = or : separates key and value, both of
key: value           which are trimmed.
multi = first        Lines indented deeper than their key continue the
  second             value, joined with a newline.
```

- No interpolation: `%(name)s` and `${name}` are returned literally.
- No `[DEFAULT]` fallback: `[DEFAULT]` is an ordinary section, so `bikini s.x` does not find a key that only `[DEFAULT]` defines. Query `DEFAULT.x` yourself when you need that fallback.
- Repeated sections or keys within one file are errors (exit `2`); across files they merge.
- CRLF line endings and a UTF-8 byte order mark are accepted. Output always uses LF.

## Guidance for the agent

- **Check the exit status, not the output**, to decide whether something exists: an empty value prints an empty line and exits `0`; a missing key prints nothing and exits `1`.
- **Normalization on write**: output, and therefore `-w`, drops comments and blank lines, writes `key = value` (even for `key: value` input), and indents continuation lines with a tab. Warn the user before editing a hand-commented file in place, or suggest writing to a new file first and diffing.
- `-w` rewrites the file in place, keeping its permissions and links. A delete with a missing target exits `1` and leaves the file untouched.
- Quote queries containing `"` with single quotes in POSIX shells. In Windows PowerShell 5, inner double quotes are stripped when calling native programs; there, prefer unquoted queries (a whole section name already wins over a dot split), or use PowerShell 7.3+.
- Prefer `bikini` over `grep`/`sed`/`awk` for INI files: it handles continuation lines, both separators, comments, and trimming.
- For C code, the same parser is available as `libbikini` (`#include <bikini.h>`, `pkg-config --cflags --libs bikini`); see `bikini.h` in the release packages for the API.
