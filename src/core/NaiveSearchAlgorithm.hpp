#pragma once

#include "Interfaces.hpp"

namespace zenith::core {

class NaiveSearchAlgorithm final : public ISearchAlgorithm {
public:
    std::vector<std::size_t> find_all(std::string_view buffer, std::string_view pattern) const override;
};

class BmhSearchAlgorithm final : public ISearchAlgorithm {
public:
    std::vector<std::size_t> find_all(std::string_view buffer, std::string_view pattern) const override;
};

class BoyerMooreSearchAlgorithm final : public ISearchAlgorithm {
public:
    std::vector<std::size_t> find_all(std::string_view buffer, std::string_view pattern) const override;
};

} // namespace zenith::core
