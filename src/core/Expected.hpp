#pragma once

#include <utility>
#include <variant>

namespace zenith::core {

template <typename T, typename E>
class Expected {
public:
    Expected(const T& value) : data_(value) {}
    Expected(T&& value) : data_(std::move(value)) {}
    Expected(const E& error) : data_(error) {}
    Expected(E&& error) : data_(std::move(error)) {}

    [[nodiscard]] bool has_value() const { return std::holds_alternative<T>(data_); }
    [[nodiscard]] explicit operator bool() const { return has_value(); }

    [[nodiscard]] T& value() { return std::get<T>(data_); }
    [[nodiscard]] const T& value() const { return std::get<T>(data_); }

    [[nodiscard]] E& error() { return std::get<E>(data_); }
    [[nodiscard]] const E& error() const { return std::get<E>(data_); }

private:
    std::variant<T, E> data_;
};

template <typename E>
class Expected<void, E> {
public:
    Expected() : has_value_(true), error_() {}
    Expected(const E& error) : has_value_(false), error_(error) {}
    Expected(E&& error) : has_value_(false), error_(std::move(error)) {}

    [[nodiscard]] bool has_value() const { return has_value_; }
    [[nodiscard]] explicit operator bool() const { return has_value_; }
    [[nodiscard]] const E& error() const { return error_; }

private:
    bool has_value_;
    E error_;
};

} // namespace zenith::core
