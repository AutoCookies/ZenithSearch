#include "NaiveSearchAlgorithm.hpp"

#include <algorithm>
#include <array>
#include <functional>

namespace zenith::core {

std::vector<std::size_t> NaiveSearchAlgorithm::find_all(std::string_view buffer, std::string_view pattern) const {
    std::vector<std::size_t> positions;
    if (pattern.empty() || buffer.size() < pattern.size()) {
        return positions;
    }

    for (std::size_t i = 0; i + pattern.size() <= buffer.size(); ++i) {
        if (buffer.compare(i, pattern.size(), pattern) == 0) {
            positions.push_back(i);
        }
    }
    return positions;
}

std::vector<std::size_t> BmhSearchAlgorithm::find_all(std::string_view buffer, std::string_view pattern) const {
    std::vector<std::size_t> positions;
    if (pattern.empty() || buffer.size() < pattern.size()) {
        return positions;
    }

    std::array<std::size_t, 256> shift;
    shift.fill(pattern.size());
    for (std::size_t i = 0; i + 1 < pattern.size(); ++i) {
        shift[static_cast<unsigned char>(pattern[i])] = pattern.size() - 1 - i;
    }

    std::size_t i = 0;
    while (i + pattern.size() <= buffer.size()) {
        std::size_t j = pattern.size();
        while (j > 0 && pattern[j - 1] == buffer[i + j - 1]) {
            --j;
        }
        if (j == 0) {
            positions.push_back(i);
            ++i; // allow overlaps
            continue;
        }
        const auto c = static_cast<unsigned char>(buffer[i + pattern.size() - 1]);
        i += std::max<std::size_t>(1, shift[c]);
    }
    return positions;
}

std::vector<std::size_t> BoyerMooreSearchAlgorithm::find_all(std::string_view buffer, std::string_view pattern) const {
    std::vector<std::size_t> positions;
    if (pattern.empty() || buffer.size() < pattern.size()) {
        return positions;
    }

    const auto searcher = std::boyer_moore_searcher(pattern.begin(), pattern.end());
    auto current = buffer.begin();
    while (current != buffer.end()) {
        auto it = std::search(current, buffer.end(), searcher);
        if (it == buffer.end()) {
            break;
        }
        const auto pos = static_cast<std::size_t>(std::distance(buffer.begin(), it));
        positions.push_back(pos);
        current = it + 1; // allow overlaps
    }
    return positions;
}

} // namespace zenith::core
