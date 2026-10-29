# Security Policy

## Supported Versions

Only the latest release receives security fixes.

## Reporting a Vulnerability

Please **do not** open a public GitHub issue for security vulnerabilities.

Use [GitHub's private vulnerability reporting](https://github.com/partoska/bikini/security/advisories/new) to submit a report. You can expect:

- Acknowledgement within **168 hours**.
- A fix or mitigation plan within **90 days**, depending on severity.
- Credit in the release notes if you would like it.

## Scope

Bikini parses INI text that may come from untrusted sources, both in the `bikini` tool and in programs linking `libbikini`. If you find input that makes the parser or writer crash, read or write out of bounds, leak memory, or loop without end, or a way for `bikini -w` to write anywhere but the file it was given, please report it.
