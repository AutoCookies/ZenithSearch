#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace zenith::core {

enum class BinaryMode { Skip, Scan };

enum class OutputMode { Matches, Count, FilesWithMatches };

struct Error {
    std::string message;
};

struct SearchRequest {
    std::string pattern;
    std::vector<std::string> input_paths;
    std::unordered_set<std::string> extensions;
    bool ignore_hidden{false};
    std::optional<std::uintmax_t> max_bytes;
    BinaryMode binary_mode{BinaryMode::Skip};
    OutputMode output_mode{OutputMode::Matches};
    bool json_output{false};
    std::size_t chunk_size{1024U * 1024U};
};

struct FileItem {
    std::string path;
    std::uintmax_t size{0};
};

struct MatchRecord {
    std::string path;
    std::uintmax_t offset{0};
    std::string snippet;
};

struct FileMatchSummary {
    std::string path;
    std::size_t count{0};
};

struct SearchStats {
    bool any_match{false};
};

} // namespace zenith::core
