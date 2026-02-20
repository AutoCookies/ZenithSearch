# ZenithSearch CLI Reference (v1.0.0)

## Usage
`zenithsearch [options] <pattern> <path...>`

## Exit codes
- `0`: at least one match
- `1`: no matches
- `2`: usage error
- `130`: cancelled (SIGINT/Ctrl+C)

## Options
- `--ext .log,.cpp,.h`
- `--ignore-hidden`
- `--exclude <glob>` (repeatable)
- `--exclude-dir <name>` (repeatable)
- `--glob <glob>` (repeatable include)
- `--no-ignore`
- `--follow-symlinks (on|off)` default `off`
- `--max-bytes N`
- `--binary (skip|scan)` default `skip`
- `--count`
- `--files-with-matches`
- `--json`
- `--max-matches N`
- `--max-snippet-bytes N` default `120`
- `--no-snippet`
- `--mmap (auto|on|off)` default `auto`
- `--threads N` default `auto` (clamped 1..32)
- `--stable-output (on|off)` default `on`
- `--algo (auto|naive|boyer_moore|bmh)` default `auto`
- `--help`
- `--version`

## Notes
- `.zenithignore` is loaded per directory unless `--no-ignore`.
- Symlink traversal cycle protection uses canonical directory path tracking.
