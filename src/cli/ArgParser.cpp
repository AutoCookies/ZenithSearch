#include "ArgParser.hpp"

#include <algorithm>
#include <cctype>
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
        if (arg == "--ext" || arg == "--max-bytes" || arg == "--binary") {
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
                try {
                    result.request.max_bytes = static_cast<std::uintmax_t>(std::stoull(value));
                } catch (...) {
                    return core::Error{"invalid --max-bytes value"};
                }
            } else if (arg == "--binary") {
                if (value == "skip") {
                    result.request.binary_mode = core::BinaryMode::Skip;
                } else if (value == "scan") {
                    result.request.binary_mode = core::BinaryMode::Scan;
                } else {
                    return core::Error{"--binary must be skip or scan"};
                }
            }
            continue;
        }

        if (!arg.empty() && arg[0] == '-') {
            return core::Error{"unknown option: " + arg};
        }

        if (result.request.pattern.empty()) {
            result.request.pattern = arg;
        } else {
            result.request.input_paths.push_back(arg);
        }
    }

    if (result.request.pattern.empty()) {
        return core::Error{"pattern is required"};
    }
    if (result.request.input_paths.empty()) {
        return core::Error{"at least one path is required"};
    }

    return result;
}

std::string ArgParser::help_text() {
    return "Usage: zenithsearch [options] <pattern> <path...>\n"
           "Options:\n"
           "  --ext .log,.cpp,.h\n"
           "  --ignore-hidden\n"
           "  --max-bytes N\n"
           "  --binary (skip|scan)\n"
           "  --count\n"
           "  --files-with-matches\n"
           "  --json\n"
           "  --help\n"
           "  --version\n";
}

} // namespace zenith::cli
