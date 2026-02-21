  <h1>ZenithSearch</h1>

<div align="center">
  <img src="assets/logo.png" alt="ZenithSearch Logo" width="180"/>

  <p>Production-ready cross-platform keyword search CLI with deterministic output, mmap acceleration, parallel scan, ignore/exclude/glob filtering, and graceful cancellation.</p>

  <a href="https://discord.gg/nnkfW83n">
    <img src="https://img.shields.io/badge/Discord-Join%20Community-5865F2?style=for-the-badge&logo=discord&logoColor=white" alt="Discord"/>
  </a>
  &nbsp;
  <img src="https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++20"/>
  &nbsp;
  <img src="https://img.shields.io/badge/CMake-3.20%2B-064F8C?style=for-the-badge&logo=cmake&logoColor=white" alt="CMake"/>
  &nbsp;
  <img src="https://img.shields.io/badge/License-NC%20Copyleft-red?style=for-the-badge" alt="License: NC Copyleft"/>
</div>

---

## Build
```bash
cmake -S . -B build
cmake --build build
```

## Test
```bash
ctest --test-dir build --output-on-failure
```

## Install / package
```bash
cmake --install build --prefix install
cpack --config build/CPackConfig.cmake
```

## Quick usage
```bash
./build/zenithsearch "TODO" src
./build/zenithsearch --exclude "**/.git/**" --exclude-dir node_modules "pattern" .
./build/zenithsearch --glob "**/*.cpp" --count "SearchEngine" src
./build/zenithsearch --mmap on --threads 8 --stable-output on "pattern" .
./build/zenithsearch --json --no-snippet "pattern" src
```

## Cancellation behavior
- Ctrl+C/SIGINT requests cancellation.
- Exit code is `130`.
- Output lines remain complete (no partial lines).
- In stable-output mode, incomplete file results are discarded.

## Output contracts
- Human match mode: `path:offset[:snippet]`
- Human count mode: `path:count`
- Human files-with-matches mode: `path`
- JSONL includes: `path`, `mode`, `pattern`, `binary`, plus:
  - `offset` (+ optional `snippet`) for match mode
  - `count` for count mode

## Symlink policy
- Default `--follow-symlinks off`.
- `on` follows symlinked directories with cycle protection.

## Ignore/exclude behavior
- `.zenithignore` files are loaded by default; disable via `--no-ignore`.
- `--exclude` uses glob over normalized `/` paths.
- `--exclude-dir` filters directories by basename.
- `--glob` acts as include filter after excludes.

## Troubleshooting
- Permission/locked file errors are written to stderr and scan continues.
- For very large trees use `--threads`, `--mmap auto`, and `--max-matches` to control memory.

See `docs/CLI.md` for full reference.
