#include "ArgParser.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>

namespace zenith::cli {

namespace {
std::string normalize_ext(std::string ext) {
    if (ext.empty()) {
        return ext;
    }
    if (ext[0] != '.') {
        ext.insert(ext.begin(), '.');
    }
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

core::Expected<std::uintmax_t, core::Error> parse_u64(const std::string& value, const std::string& flag) {
    std::uintmax_t out = 0;
    const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), out);
    if (ec != std::errc{} || ptr != value.data() + value.size()) {
        return core::Error{"invalid " + flag + " value"};
    }
    return out;
}
} // namespace

core::Expected<ParseResult, core::Error> ArgParser::parse(const std::vector<std::string>& args) const {
    ParseResult result;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const auto& arg = args[i];
        if (arg == "--help") {
            result.show_help = true;
            return result;
        }
        if (arg == "--version") {
            result.show_version = true;
            return result;
        }
        if (arg == "--ignore-hidden") {
            result.request.ignore_hidden = true;
            continue;
        }
        if (arg == "--no-ignore") {
            result.request.no_ignore = true;
            continue;
        }
        if (arg == "--count") {
            if (result.request.output_mode == core::OutputMode::FilesWithMatches) {
                return core::Error{"--count conflicts with --files-with-matches"};
            }
            result.request.output_mode = core::OutputMode::Count;
            continue;
        }
        if (arg == "--files-with-matches") {
            if (result.request.output_mode == core::OutputMode::Count) {
                return core::Error{"--files-with-matches conflicts with --count"};
            }
            result.request.output_mode = core::OutputMode::FilesWithMatches;
            continue;
        }
        if (arg == "--json") {
            result.request.json_output = true;
            continue;
        }
        if (arg == "--no-snippet") {
            result.request.no_snippet = true;
            continue;
        }

        if (arg == "--ext" || arg == "--max-bytes" || arg == "--binary" || arg == "--mmap" || arg == "--threads" ||
            arg == "--stable-output" || arg == "--algo" || arg == "--exclude" || arg == "--exclude-dir" || arg == "--glob" ||
            arg == "--follow-symlinks" || arg == "--max-matches" || arg == "--max-snippet-bytes") {
            if (i + 1 >= args.size()) {
                return core::Error{"missing value for " + arg};
            }
            const auto value = args[++i];
            if (arg == "--ext") {
                std::stringstream ss(value);
                std::string ext;
                while (std::getline(ss, ext, ',')) {
                    ext = normalize_ext(ext);
                    if (!ext.empty()) {
                        result.request.extensions.insert(ext);
                    }
                }
            } else if (arg == "--max-bytes") {
                auto parsed = parse_u64(value, "--max-bytes");
                if (!parsed) return parsed.error();
                result.request.max_bytes = parsed.value();
            } else if (arg == "--threads") {
                auto parsed = parse_u64(value, "--threads");
                if (!parsed) return parsed.error();
                result.request.threads = static_cast<std::size_t>(parsed.value());
            } else if (arg == "--max-matches") {
                auto parsed = parse_u64(value, "--max-matches");
                if (!parsed) return parsed.error();
                result.request.max_matches_per_file = static_cast<std::size_t>(parsed.value());
            } else if (arg == "--max-snippet-bytes") {
                auto parsed = parse_u64(value, "--max-snippet-bytes");
                if (!parsed) return parsed.error();
                result.request.max_snippet_bytes = static_cast<std::size_t>(parsed.value());
            } else if (arg == "--binary") {
                if (value == "skip") result.request.binary_mode = core::BinaryMode::Skip;
                else if (value == "scan") result.request.binary_mode = core::BinaryMode::Scan;
                else return core::Error{"--binary must be skip or scan"};
            } else if (arg == "--mmap") {
                if (value == "auto") result.request.mmap_mode = core::MmapMode::Auto;
                else if (value == "on") result.request.mmap_mode = core::MmapMode::On;
                else if (value == "off") result.request.mmap_mode = core::MmapMode::Off;
                else return core::Error{"--mmap must be auto, on, or off"};
            } else if (arg == "--stable-output") {
                if (value == "on") result.request.stable_output = core::StableOutputMode::On;
                else if (value == "off") result.request.stable_output = core::StableOutputMode::Off;
                else return core::Error{"--stable-output must be on or off"};
            } else if (arg == "--algo") {
                if (value == "auto") result.request.algorithm_mode = core::AlgorithmMode::Auto;
                else if (value == "naive") result.request.algorithm_mode = core::AlgorithmMode::Naive;
                else if (value == "boyer_moore") result.request.algorithm_mode = core::AlgorithmMode::BoyerMoore;
                else if (value == "bmh") result.request.algorithm_mode = core::AlgorithmMode::Bmh;
                else return core::Error{"--algo must be auto, naive, boyer_moore, or bmh"};
            } else if (arg == "--exclude") {
                result.request.exclude_globs.push_back(value);
            } else if (arg == "--exclude-dir") {
                result.request.exclude_dirs.push_back(value);
            } else if (arg == "--glob") {
                result.request.include_globs.push_back(value);
            } else if (arg == "--follow-symlinks") {
                if (value == "on") result.request.follow_symlinks = core::FollowSymlinksMode::On;
                else if (value == "off") result.request.follow_symlinks = core::FollowSymlinksMode::Off;
                else return core::Error{"--follow-symlinks must be on or off"};
            }
            continue;
        }

        if (!arg.empty() && arg[0] == '-') {
            return core::Error{"unknown option: " + arg};
        }

        if (result.request.pattern.empty()) result.request.pattern = arg;
        else result.request.input_paths.push_back(arg);
    }

    if (result.request.pattern.empty()) return core::Error{"pattern is required"};
    if (result.request.input_paths.empty()) return core::Error{"at least one path is required"};

    return result;
}

std::string ArgParser::help_text() {
    return "Usage: zenithsearch [options] <pattern> <path...>\n"
           "Options:\n"
           "  --ext .log,.cpp,.h\n"
           "  --ignore-hidden\n"
           "  --exclude <glob> (repeatable)\n"
           "  --exclude-dir <name> (repeatable)\n"
           "  --glob <glob> (repeatable include)\n"
           "  --no-ignore\n"
           "  --follow-symlinks (on|off) [default: off]\n"
           "  --max-bytes N\n"
           "  --binary (skip|scan) [default: skip]\n"
           "  --count\n"
           "  --files-with-matches\n"
           "  --json\n"
           "  --max-matches N [default: unlimited]\n"
           "  --max-snippet-bytes N [default: 120]\n"
           "  --no-snippet\n"
           "  --mmap (auto|on|off) [default: auto]\n"
           "  --threads N [default: auto]\n"
           "  --stable-output (on|off) [default: on]\n"
           "  --algo (auto|naive|boyer_moore|bmh) [default: auto]\n"
           "  --help\n"
           "  --version\n";
}

} // namespace zenith::cli
