#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace doctest {
using TestFunc = std::function<void()>;
inline std::vector<std::pair<std::string, TestFunc>>& registry() {
    static std::vector<std::pair<std::string, TestFunc>> tests;
    return tests;
}
struct Registrar {
    Registrar(std::string name, TestFunc fn) { registry().emplace_back(std::move(name), std::move(fn)); }
};
inline int run_tests() {
    int failed = 0;
    for (const auto& [name, fn] : registry()) {
        try {
            fn();
        } catch (const std::exception& ex) {
            std::cerr << "[FAIL] " << name << ": " << ex.what() << '\n';
            ++failed;
        }
    }
    if (failed == 0) {
        std::cout << "All tests passed (" << registry().size() << ")\n";
    }
    return failed == 0 ? 0 : 1;
}
} // namespace doctest

#define DOCTEST_CONCAT_IMPL(s1, s2) s1##s2
#define DOCTEST_CONCAT(s1, s2) DOCTEST_CONCAT_IMPL(s1, s2)
#define TEST_CASE(name) \
    static void DOCTEST_CONCAT(test_, __LINE__)(); \
    static doctest::Registrar DOCTEST_CONCAT(reg_, __LINE__)(name, DOCTEST_CONCAT(test_, __LINE__)); \
    static void DOCTEST_CONCAT(test_, __LINE__)()

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            throw std::runtime_error(std::string("CHECK failed: ") + #expr); \
        } \
    } while (false)

#define REQUIRE(expr) CHECK(expr)

#define CHECK_FALSE(expr) CHECK(!(expr))
#define REQUIRE_FALSE(expr) REQUIRE(!(expr))
