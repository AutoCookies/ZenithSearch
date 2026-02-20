# Changelog

## v1.0.0
- Added production filtering pipeline: `--exclude`, `--exclude-dir`, `--glob`, `.zenithignore`, and `--no-ignore`.
- Added explicit symlink policy: `--follow-symlinks on|off` with cycle protection.
- Added output limiting controls: `--max-matches`, `--max-snippet-bytes`, `--no-snippet`.
- Added JSONL output contract fields: `path`, `mode`, `pattern`, `binary`, plus mode-specific fields.
- Added graceful cancellation handling for SIGINT/Ctrl+C with exit code `130`.
- Added install and packaging support via CMake install + CPack.
- Added release workflow producing artifacts for Linux/macOS/Windows.
- Expanded tests for filtering, globs, symlink/cancel behavior, and golden-output stability.

## v0.2.0
- Added mmap and parallel scanning baseline.
