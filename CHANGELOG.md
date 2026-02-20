# Changelog

## v0.2.0
- Added cross-platform memory-mapped file support with RAII wrappers and fallback behavior.
- Added parallel scanning with configurable `--threads` and deterministic `--stable-output` mode.
- Added search algorithm strategies: `naive`, `bmh`, `boyer_moore`, and `auto` selection.
- Kept streaming mode with chunk-overlap correctness for cross-chunk matches.
- Expanded tests for mmap, algorithm equivalence, and parallel determinism/robustness.
- Updated CI with a smoke benchmark step.

## v0.1.0
- Baseline streaming chunked scanner with overlap handling.
- Recursive traversal with extension/max-size/hidden filtering.
- Binary detection with `--binary skip|scan`.
- Human and JSONL output modes (`--count`, `--files-with-matches`).
- Doctest test suite and GitHub Actions CI for Ubuntu/macOS/Windows.
