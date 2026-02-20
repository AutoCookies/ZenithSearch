#include "cli/ArgParser.hpp"

#include "doctest.h"

TEST_CASE("ArgParser parses v1 options") {
    zenith::cli::ArgParser parser;
    auto parsed = parser.parse({"--exclude", "**/.git/**", "--exclude-dir", "node_modules", "--glob", "**/*.cpp", "--no-ignore",
                                "--follow-symlinks", "on", "--max-matches", "10", "--max-snippet-bytes", "64", "--no-snippet", "pat", "."});
    REQUIRE(parsed.has_value());
    CHECK(parsed.value().request.exclude_globs.size() == 1);
    CHECK(parsed.value().request.exclude_dirs.size() == 1);
    CHECK(parsed.value().request.include_globs.size() == 1);
    CHECK(parsed.value().request.no_ignore);
    CHECK(parsed.value().request.follow_symlinks == zenith::core::FollowSymlinksMode::On);
    CHECK(parsed.value().request.max_matches_per_file.value() == 10);
    CHECK(parsed.value().request.max_snippet_bytes == 64);
    CHECK(parsed.value().request.no_snippet);
}

TEST_CASE("ArgParser rejects conflicts") {
    zenith::cli::ArgParser parser;
    auto parsed = parser.parse({"--count", "--files-with-matches", "pat", "."});
    REQUIRE_FALSE(parsed.has_value());
}
