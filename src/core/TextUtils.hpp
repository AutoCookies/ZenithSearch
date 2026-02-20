#pragma once

#include <cctype>
#include <string>

namespace zenith::core {

inline std::string sanitize_snippet(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (unsigned char c : input) {
        if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else if (c == '\t') {
            out += "\\t";
        } else if (std::isprint(c) != 0) {
            out.push_back(static_cast<char>(c));
        } else {
            out += "..";
        }
    }
    return out;
}

inline std::string json_escape(const std::string& input) {
    std::string out;
    for (unsigned char c : input) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (c < 0x20) {
                out += "..";
            } else {
                out.push_back(static_cast<char>(c));
            }
        }
    }
    return out;
}

} // namespace zenith::core
