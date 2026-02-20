#pragma once

#include "core/Expected.hpp"
#include "core/Types.hpp"

#include <string>
#include <vector>

namespace zenith::cli {

struct ParseResult {
    core::SearchRequest request;
    bool show_help{false};
    bool show_version{false};
};

class ArgParser {
public:
    core::Expected<ParseResult, core::Error> parse(const std::vector<std::string>& args) const;
    static std::string help_text();
};

} // namespace zenith::cli
