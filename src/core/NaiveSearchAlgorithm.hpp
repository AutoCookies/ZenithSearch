#pragma once

#include "Interfaces.hpp"

namespace zenith::core {

class NaiveSearchAlgorithm final : public ISearchAlgorithm {
public:
    std::vector<std::size_t> find_all(const std::string& buffer, const std::string& pattern) const override;
};

} // namespace zenith::core
