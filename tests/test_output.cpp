#include "platform/OutputWriters.hpp"

#include "doctest.h"

#include <sstream>

TEST_CASE("Human output formatting") {
    std::ostringstream os;
    zenith::platform::HumanOutputWriter writer(os, zenith::core::OutputMode::Matches);
    writer.write_match({"p", 42, "snip"});
    CHECK(os.str() == "p:42:snip\n");
}

TEST_CASE("Count output formatting") {
    std::ostringstream os;
    zenith::platform::HumanOutputWriter writer(os, zenith::core::OutputMode::Count);
    writer.write_file_summary({"p", 3});
    CHECK(os.str() == "p:3\n");
}

TEST_CASE("JSONL output formatting") {
    std::ostringstream os;
    zenith::platform::JsonlOutputWriter writer(os, zenith::core::OutputMode::Matches);
    writer.write_match({"path", 1, "a\n"});
    const auto line = os.str();
    CHECK(line.find("{\"type\":\"match\"") == 0);
    CHECK(line.find("\"path\":\"path\"") != std::string::npos);
}
