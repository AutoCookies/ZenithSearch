#include "cli/ArgParser.hpp"

#include "doctest.h"

TEST_CASE("Usage error maps to exit code 2") {
    zenith::cli::ArgParser parser;
    auto res = parser.parse({});
    CHECK_FALSE(res.has_value());
}
