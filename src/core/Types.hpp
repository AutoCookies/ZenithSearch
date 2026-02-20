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
enum class MmapMode { Auto, On, Off };
enum class StableOutputMode { On, Off };
enum class AlgorithmMode { Auto, Naive, BoyerMoore, Bmh };
enum class FollowSymlinksMode { Off, On };

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

    MmapMode mmap_mode{MmapMode::Auto};
    std::size_t mmap_threshold_bytes{64U * 1024U};
    std::size_t threads{0}; // 0 = auto
    StableOutputMode stable_output{StableOutputMode::On};
    AlgorithmMode algorithm_mode{AlgorithmMode::Auto};

    std::vector<std::string> exclude_globs;
    std::vector<std::string> exclude_dirs;
    std::vector<std::string> include_globs;
    bool no_ignore{false};
    FollowSymlinksMode follow_symlinks{FollowSymlinksMode::Off};

    std::optional<std::size_t> max_matches_per_file;
    std::size_t max_snippet_bytes{120};
    bool no_snippet{false};
};

struct FileItem {
    std::string path;
    std::string normalized_path;
    std::uintmax_t size{0};
};

struct MatchRecord {
    std::string path;
    std::uintmax_t offset{0};
    std::string snippet;
    bool binary{false};
};

struct FileMatchSummary {
    std::string path;
    std::size_t count{0};
    bool binary{false};
};

struct FileResult {
    std::string path;
    std::vector<MatchRecord> matches;
    std::size_t count{0};
    bool any_match{false};
    bool binary{false};
    bool completed{true};
};

struct SearchStats {
    bool any_match{false};
    bool cancelled{false};
};

} // namespace zenith::core
