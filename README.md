# ZenithSearch (Phase 2)

ZenithSearch is a cross-platform keyword search CLI (minimal grep-like tool) with deterministic behavior, mmap acceleration, and parallel scanning.

## Features
- Recursive scan across files/directories.
- Streaming file read (1MB chunks) and memory-mapped scanning.
- Parallel scanning with configurable thread count.
- Deterministic stable output ordering (default on).
- Byte-based, case-sensitive substring search with multiple algorithms:
  - `naive`
  - `bmh` (Boyer-Moore-Horspool)
  - `boyer_moore`
  - `auto`
- Binary detection heuristic (first 4KB contains `\0`).
- Human and JSONL output.
- Exit codes: `0` match found, `1` no matches, `2` usage/fatal error.

## Build (Linux/macOS/Windows)
```bash
cmake -S . -B build
cmake --build build
```

## Test
```bash
ctest --test-dir build --output-on-failure
```

## Run
```bash
# basic search
./build/zenithsearch "TODO" src

# extension filter + count
./build/zenithsearch --ext .cpp,.h --count "SearchEngine" src

# mmap + parallel
./build/zenithsearch --mmap on --threads 8 "pattern" .

# algorithm selection
./build/zenithsearch --algo bmh "pattern" src

# fast unstable output mode
./build/zenithsearch --stable-output off --threads 8 "pattern" src

# JSONL mode
./build/zenithsearch --json "pattern" src
```

Windows (PowerShell):
```powershell
.\build\zenithsearch.exe "TODO" src
```

## `--help` output
```text
Usage: zenithsearch [options] <pattern> <path...>
Options:
  --ext .log,.cpp,.h
  --ignore-hidden
  --max-bytes N
  --binary (skip|scan) [default: skip]
  --count
  --files-with-matches
  --json
  --mmap (auto|on|off) [default: auto]
  --threads N [default: auto]
  --stable-output (on|off) [default: on]
  --algo (auto|naive|boyer_moore|bmh) [default: auto]
  --help
  --version
```

## Project layout
- `src/core`: DTOs, expected type, algorithms, search engine.
- `src/platform`: filesystem enumeration, file reader, mmap providers, output writers.
- `src/cli`: argument parser.
- `tests`: header-only test suite.
- `.github/workflows/ci.yml`: CI matrix for Linux/macOS/Windows + smoke benchmark.

## Roadmap
Phase 3 may add indexing and advanced query capabilities.
