#include "platform/OutputWriters.hpp"

#include "doctest.h"

#include <sstream>

TEST_CASE("Human output supports no snippet") {
    std::ostringstream os;
    zenith::platform::HumanOutputWriter writer(os, zenith::core::OutputMode::Matches, true);
    writer.write_match({"p", 42, "snip", false});
    CHECK(os.str() == "p:42\n");
}

TEST_CASE("JSON output contract includes mode and pattern") {
    std::ostringstream os;
    zenith::platform::JsonlOutputWriter writer(os, zenith::core::OutputMode::Matches, "pat", false);
    writer.write_match({"path", 1, "a", false});
    const auto line = os.str();
    CHECK(line.find("\"mode\":\"match\"") != std::string::npos);
    CHECK(line.find("\"pattern\":\"pat\"") != std::string::npos);
}
