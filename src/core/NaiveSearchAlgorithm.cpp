#include "NaiveSearchAlgorithm.hpp"

#include <algorithm>

namespace zenith::core {

std::vector<std::size_t> NaiveSearchAlgorithm::find_all(const std::string& buffer, const std::string& pattern) const {
    std::vector<std::size_t> positions;
    if (pattern.empty() || buffer.size() < pattern.size()) {
        return positions;
    }

    auto it = buffer.begin();
    while (it != buffer.end()) {
        it = std::search(it, buffer.end(), pattern.begin(), pattern.end());
        if (it == buffer.end()) {
            break;
        }
        positions.push_back(static_cast<std::size_t>(std::distance(buffer.begin(), it)));
        ++it;
    }
    return positions;
}

} // namespace zenith::core
