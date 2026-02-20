#include "cli/ArgParser.hpp"

#include "doctest.h"

TEST_CASE("ArgParser requires pattern and path") {
    zenith::cli::ArgParser parser;
    auto parsed = parser.parse({"--count"});
    REQUIRE_FALSE(parsed.has_value());
}

TEST_CASE("ArgParser normalizes extensions") {
    zenith::cli::ArgParser parser;
    auto parsed = parser.parse({"--ext", "cpp,.H", "pat", "."});
    REQUIRE(parsed.has_value());
    CHECK(parsed.value().request.extensions.contains(".cpp"));
    CHECK(parsed.value().request.extensions.contains(".h"));
}

TEST_CASE("ArgParser parses phase2 options") {
    zenith::cli::ArgParser parser;
    auto parsed = parser.parse({"--mmap", "on", "--threads", "8", "--stable-output", "off", "--algo", "bmh", "pat", "."});
    REQUIRE(parsed.has_value());
    CHECK(parsed.value().request.mmap_mode == zenith::core::MmapMode::On);
    CHECK(parsed.value().request.threads == 8);
    CHECK(parsed.value().request.stable_output == zenith::core::StableOutputMode::Off);
    CHECK(parsed.value().request.algorithm_mode == zenith::core::AlgorithmMode::Bmh);
}

TEST_CASE("ArgParser rejects conflicting mode flags") {
    zenith::cli::ArgParser parser;
    auto parsed = parser.parse({"--count", "--files-with-matches", "pat", "."});
    REQUIRE_FALSE(parsed.has_value());
}
