#include "platform/Glob.hpp"

#include "doctest.h"

TEST_CASE("glob supports star question and doublestar") {
    CHECK(zenith::platform::glob_match("**/*.cpp", "a/b/c.cpp"));
    CHECK(zenith::platform::glob_match("src/?ain.cpp", "src/main.cpp"));
    CHECK_FALSE(zenith::platform::glob_match("src/*.cpp", "src/a/b.cpp"));
}
