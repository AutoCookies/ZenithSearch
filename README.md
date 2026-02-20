# ZenithSearch (Phase 1)

ZenithSearch is a cross-platform, production-grade baseline keyword search CLI (minimal grep-like tool) focused on clean architecture and deterministic behavior.

## Features
- Recursive scan across files/directories.
- Streaming file read (default 1MB chunks) with cross-chunk match support.
- Byte-based, case-sensitive substring search.
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

# extension filter
./build/zenithsearch --ext .cpp,.h "SearchEngine" src

# count mode
./build/zenithsearch --count "pattern" .

# files-with-matches mode
./build/zenithsearch --files-with-matches "pattern" .

# JSONL mode
./build/zenithsearch --json "pattern" src
```

Windows (PowerShell):
```powershell
.\build\zenithsearch.exe "TODO" src
```

## `--help` example output
```text
Usage: zenithsearch [options] <pattern> <path...>
Options:
  --ext .log,.cpp,.h
  --ignore-hidden
  --max-bytes N
  --binary (skip|scan)
  --count
  --files-with-matches
  --json
  --help
  --version
```

## Project layout
- `src/core`: interfaces, DTOs, expected type, search engine, algorithm.
- `src/platform`: filesystem enumeration, file reader, stdout/stderr writers.
- `src/cli`: argument parser.
- `tests`: Doctest-based test suite.
- `.github/workflows/ci.yml`: CI matrix for Linux/macOS/Windows.

## Roadmap
Phase 2 will add mmap-based IO, parallelism, and faster search algorithms.
