#pragma once

#include <string>

namespace zenith::platform {

inline std::string normalize_for_match(std::string p) {
    for (char& c : p) {
        if (c == '\\') {
            c = '/';
        }
    }
    return p;
}

inline bool glob_match_impl(const std::string& pattern, std::size_t pi, const std::string& text, std::size_t ti) {
    while (pi < pattern.size()) {
        if (pattern[pi] == '*') {
            const bool doublestar = (pi + 1 < pattern.size() && pattern[pi + 1] == '*');
            if (doublestar) {
                pi += 2;
                while (pi < pattern.size() && pattern[pi] == '*') {
                    ++pi;
                }
                if (pi == pattern.size()) {
                    return true;
                }
                for (std::size_t k = ti; k <= text.size(); ++k) {
                    if (glob_match_impl(pattern, pi, text, k)) {
                        return true;
                    }
                }
                return false;
            }
            ++pi;
            for (std::size_t k = ti; k <= text.size(); ++k) {
                if (k > ti && text[k - 1] == '/') {
                    break;
                }
                if (glob_match_impl(pattern, pi, text, k)) {
                    return true;
                }
            }
            return false;
        }
        if (pattern[pi] == '?') {
            if (ti >= text.size() || text[ti] == '/') {
                return false;
            }
            ++pi;
            ++ti;
            continue;
        }
        if (ti >= text.size() || pattern[pi] != text[ti]) {
            return false;
        }
        ++pi;
        ++ti;
    }
    return ti == text.size();
}

inline bool glob_match(const std::string& pattern, const std::string& text) {
    return glob_match_impl(normalize_for_match(pattern), 0, normalize_for_match(text), 0);
}

} // namespace zenith::platform
