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

TEST_CASE("ArgParser rejects conflicting mode flags") {
    zenith::cli::ArgParser parser;
    auto parsed = parser.parse({"--count", "--files-with-matches", "pat", "."});
    REQUIRE_FALSE(parsed.has_value());
}
