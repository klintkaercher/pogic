#pragma once

#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace khelper {

using u8    = uint8_t;
using u16   = uint16_t;
using u32   = uint32_t;
using u64   = uint64_t;
using usize = uint64_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using String = std::string;
#if __cplusplus >= 201703L
using StringV = std::string_view;
#else
using StringV = std::string;
#endif

template <typename T>
using Vec = std::vector<T>;

#if __cplusplus >= 201703L
template <typename T>
using Option = std::optional<T>;
#endif

using std::make_optional;

template <typename T>
using SPtr = std::shared_ptr<T>;
using std::make_shared;

template <typename T>
using UPtr = std::unique_ptr<T>;
using std::make_unique;

using std::literals::string_literals::operator""s;

// RESULT
struct BadResultOkAccess : public std::exception {};
struct BadResultErrAccess : public std::exception {};

template <typename T, typename E>
struct Result {
    operator bool() {
        return (bool)this->value_;
    }

    auto value() -> T {
        if (this) {
            return this->value_.value();
        } else {
            throw BadResultOkAccess();
        }
    }

    auto err_value() -> E {
        if (this->err_value_) {
            return this->err_value_.value();
        } else {
            throw BadResultErrAccess();
        }
    }

    template <typename U, typename Transform>
    auto transform(Transform func) -> Result<U, E> {
        if (this) {
            return Result<U, E>{
                .value_     = func(this->value_.value()),
                .err_value_ = this->err_value_,
            };
        } else {
            return Result<U, E>{
                .value_     = {},
                .err_value_ = this->err_value_,
            };
        }
    }

    template <typename U, typename Transform>
    auto err_transform(Transform func) -> Result<T, U> {
        if (this->err_value_) {
            return Result<T, U>{
                .value_     = {},
                .err_value_ = func(this->err_value_.value()),
            };
        } else {
            return Result<T, U>{
                .value_     = this->value_,
                .err_value_ = {},
            };
        }
    }

    template <typename U>
    auto value_or(U alternative) -> U {
        if (this) {
            return this->value_.value();
        } else {
            return alternative;
        }
    }

    std::optional<T> value_     = std::nullopt;
    std::optional<E> err_value_ = std::nullopt;
};

template <typename T, typename E>
auto Ok(T input) -> Result<T, E> {
    return Result<T, E>{.value_ = std::make_optional(input)};
}

template <typename T, typename E>
auto Err(E input) -> Result<T, E> {
    return Result<T, E>{.err_value_ = std::make_optional(input)};
}

/// PARSE
auto parse_i32(const std::string &input) -> std::optional<int32_t>;
auto parse_i64(const std::string &input) -> std::optional<int64_t>;
auto parse_u32(const std::string &input) -> std::optional<uint32_t>;
auto parse_u64(const std::string &input) -> std::optional<uint64_t>;

/// STRING
template <typename Predicate>
auto find_char(Predicate p, const std::string_view input)
    -> std::optional<std::pair<size_t, char>> {
    for (size_t i = 0; i < input.size(); i++) {
        if (p(input[i])) { return std::make_pair(i, input[i]); }
    }
    return {};
}

auto args_vec(int argc, const char **argv) -> std::vector<std::string>;
auto string_break(const std::string_view input) -> std::vector<std::string>;
auto quote_string(const std::string_view input) -> std::string;
auto split(const std::string_view input, const char &delim)
    -> std::vector<std::string>;
auto find(const std::string_view needle, const std::string_view haystack)
    -> std::optional<size_t>;
auto replace(const std::string_view input, const std::string_view from,
             const std::string_view to) -> std::string;
auto replacen(const std::string_view input, const std::string_view from,
              const std::string_view to, const size_t max_count) -> std::string;
auto slice(const size_t start, const std::string &input) -> std::string;
auto slice(const size_t start, const size_t end, const std::string &input)
    -> std::string;
auto lines(const std::string &input) -> std::vector<std::string>;
auto starts_with(const std::string_view needle, const std::string_view haystack)
    -> bool;
auto ends_with(const std::string_view needle, const std::string_view haystack)
    -> bool;
auto to_lowercase(const std::string_view input) -> std::string;
auto to_uppercase(const std::string_view input) -> std::string;
auto strip_prefix(const std::string_view prefix, const std::string_view input)
    -> std::optional<std::string>;
auto strip_suffix(const std::string_view suffix, const std::string_view input)
    -> std::optional<std::string>;

// FORMATTING
template <typename T>
auto format(const std::string_view fmt_string, T input) -> std::string {
    if (!find("{}", fmt_string)) { return std::string{fmt_string}; }

    std::ostringstream oss;
    if (auto it = find("{}", fmt_string); it) {
        oss << fmt_string.substr(0, it.value());
        oss << input;
        oss << ((it.value() + 2 < fmt_string.size())
                    ? fmt_string.substr(it.value() + 2)
                    : "");
    }
    return oss.str();
}

template <typename T, typename... Args>
auto format(const std::string_view fmt_string, T first, Args... rest)
    -> std::string {
    std::string processed = format(fmt_string, first);

    return format(processed, rest...);
}

// VECTOR

template <typename T>
auto nth(const size_t index, const std::vector<T> &input) -> std::optional<T> {
    if (index < input.size()) { return std::make_optional(input[index]); }
    return {};
}

template <typename Transform, typename T, typename U>
auto flat_map(Transform func, std::vector<T> input) -> std::vector<U> {
    std::vector<U> output = {};
    for (const auto &it : input) {
        if (auto elem = func(it); elem) { output.push_back(elem.value()); }
    }
    return output;
}

/// F is f(acc, elem) -> new_acc.
template <typename T, typename U, typename F>
auto fold(U init, F func, std::vector<T> input) -> U {
    U acc{init};
    for (const auto &it : input) {
        acc = func(acc, it);
    }
    return acc;
}

/// F is f(acc, elem) -> <Some type that converts to bool>.
template <typename T, typename U, typename F>
auto try_fold(U init, F func, std::vector<T> input) -> U {
    U acc{init};
    for (const auto &it : input) {
        if (auto new_acc = func(acc, it); new_acc) {
            acc = new_acc;
        } else {
            return acc;
        }
    }
    return acc;
}

template <typename T>
auto slice(size_t start, size_t end, std::vector<T> input)
    -> std::optional<std::vector<T>> {
    if (end <= start) { return {}; }
    if (start >= input.size()) { return {}; }

    std::vector<T> output = {};
    for (size_t i = start; i < (end <= input.size()) ? end : input.size();
         i++) {
        output.push_back(input[i]);
    }
    return output;
}

template <typename T>
auto slice(size_t start, std::vector<T> input)
    -> std::optional<std::vector<T>> {
    return slice(input, start, input.size());
}

template <typename Predicate, typename T>
auto take_while(Predicate pred, const std::vector<T> &input) -> std::vector<T> {
    std::vector<T> output = {};
    for (const auto &it : input) {
        if (pred(it)) {
            output.push_back(it);
        } else {
            break;
        }
    }

    return output;
}

// `destination` is being extended by `source`.
template <typename T, typename U>
auto append(std::vector<T> &dest, const std::vector<U> &src) {
    dest.reserve(dest.size() + src.size());
    dest.insert(dest.end(), src.begin(), src.end());
}

// Takes two vectors and returns them concatenated.
// While it can take two types, the output type is determined by the type of the
// first vector.
template <typename T, typename U>
auto concat(const std::vector<T> &first, const std::vector<U> &second)
    -> std::vector<T> {
    std::vector<T> output(first.size() + second.size());
    for (size_t i = 0; i < first.size(); i++) {
        output.push_back(first[i]);
    }
    for (size_t i = 0; i < second.size(); i++) {
        output.push_back(second[i]);
    }
    return output;
}

template <typename T>
auto flatten(const std::vector<std::optional<T>> &input) -> std::vector<T> {
    std::vector<T> output = {};
    for (size_t i = 0; i < input.size(); i++) {
        if (input[i]) { output.push_back(input[i].value()); }
    }
    return output;
}

template <typename T>
auto flatten(const std::vector<std::vector<T>> &input) -> std::vector<T> {
    std::vector<T> output = {};
    for (const auto &it : input) {
        for (const auto &elem : it) {
            output.push_back(elem);
        }
    }
    return output;
}

template <typename T, typename U, typename Transform>
auto fmap(Transform func, const std::vector<T> &input) -> std::vector<U> {
    std::vector<T> output = {};
    for (const auto &it : input) {
        output.push_back(func(it));
    }
    return output;
}

template <typename T, typename Predicate>
auto filter(Predicate pred, const std::vector<T> &input) -> std::vector<T> {
    std::vector<T> output = {};
    for (const auto &it : input) {
        if (pred(it)) { output.push_back(it); }
    }
    return output;
}

template <typename T, typename Predicate>
auto retain(Predicate pred, std::vector<T> &input) -> std::vector<T> {
    std::vector<size_t> remove_indices = {};
    for (size_t i = 0; i < input.size(); i++) {
        if (!pred(input[i])) { remove_indices.push_back(i); }
    }

    for (size_t i = 0; i < remove_indices.size(); i++) {
        input.erase(remove_indices[remove_indices.size() - 1 - i]);
    }
}

template <typename T>
auto find(const T &needle, const std::vector<T> &haystack)
    -> std::optional<size_t> {
    for (size_t i = 0; i < haystack.size(); i++) {
        if (haystack[i] == needle) { return i; }
    }

    return {};
}

template <typename T>
auto after(const T &lead_value, const std::vector<T> &input)
    -> std::optional<T> {
    if (auto lead_idx = find(lead_value, input); lead_idx) {
        return nth(lead_idx.value() + 1, input);
    }
    return {};
}

// OPTIONAL
#if __cplusplus < 201703L
// Maybe this should use heap allocations and a raw pointer for the value?
// So None values can just have a nullptr?
template <typename T>
struct Option {
    bool has_value() {
        return this->has_value_;
    }

  private:
    T value_;
    bool has_value_;
};

template <typename T>
auto Some(T input) -> Option<T> {
}

template <typename T>
auto None() -> Option<T> {
    return Option{.value_ = T(), .has_value_ = false};
}
#endif

struct ExpectedOptionalValue : std::exception {
    explicit ExpectedOptionalValue(const char *input) : value_(input) {
    }
    const char *what() const noexcept {
        return value_;
    }
    const char *value_;
};

template <typename T>
auto expect(const std::optional<T> &input, std::string error_msg) -> T {
    if (input) { return input.value(); }
    throw ExpectedOptionalValue(error_msg.c_str());
}

// Right now this assumes that `f()` returns some sort of std::optional<T>.
template <typename T, typename U, typename Callable>
auto or_else(const std::optional<T> &input, Callable call) -> U {
    return (input) ? input.value() : call();
}

// Right now this assumes that `f()` returns some sort of std::optional<T>.
// Calls `fmap` under the hood.
template <typename T, typename U, typename Transform>
auto transform(const std::optional<T> &input, Transform func)
    -> std::optional<U> {
    return (input) ? func(input.value()) : std::nullopt;
}

template <typename T, typename U, typename Transform>
auto and_then(const std::optional<T> &input, Transform func) -> U {
    return (input) ? func(input.value()) : std::nullopt;
}

// PRINTING

template <typename T>
auto operator<<(std::ostream &os, std::optional<T> rhs) -> std::ostream & {
    if (rhs) {
        os << "Some(" << rhs.value() << ")";
    } else {
        os << "None";
    }
    return os;
}

template <typename T>
auto operator<<(std::ostream &os, std::vector<T> rhs) -> std::ostream & {
    os << "Vec { ";
    for (size_t i = 0; i < rhs.size(); i++) {
        os << rhs[i];
        if (i != rhs.size() - 1) { os << ", "; }
    }
    os << " }";
    return os;
}

template <typename T>
auto to_string(std::optional<T> input) -> std::string {
    std::ostringstream os;
    os << input;
    return os.str();
}

template <typename T, typename E>
auto operator<<(std::ostream &os, Result<T, E> input) -> std::ostream & {
    if (input) {
        os << "Ok(" << input.value() << ")";
    } else {
        os << "Err(" << input.err_value() << ")";
    }
    return os;
}

template <typename T, typename E>
auto to_string(Result<T, E> input) -> std::string {
    std::ostringstream oss;
    oss << input;
    return oss.str();
}

auto operator<<(std::ostream &os, std::vector<uint8_t> input) -> std::ostream &;

template <typename T>
auto operator<<(std::ostream &os, std::set<T> input) -> std::ostream & {
    os << "Set { ";
    std::string tmp = "";
    for (const auto &it : input) {
        tmp += it + ", ";
    }
    os << tmp.substr(0, tmp.size() - 2);
    os << " }";
    return os;
}

template <typename T>
auto to_string(std::set<T> input) -> std::string {
    std::ostringstream oss;
    oss << input;
    return oss.str();
}

auto println();
auto eprintln();

template <typename T>
auto println(const T &input) {
    std::cout << input << std::endl;
}

template <typename... Args>
auto println(const std::string_view fmt_string, Args... args) {
    std::cout << format(fmt_string, args...) << std::endl;
}

template <typename T>
auto eprintln(const T &input) {
    std::cerr << input << std::endl;
}

template <typename T>
auto debug(const T &input) -> const T & {
    println(input);
    return input;
}

template <typename T>
auto debug(T &input) -> T & {
    println(input);
    return input;
}

// TESTING
#define kassert(x) assert_helper(x, "", __FILE__, __LINE__)
#define kassert_msg(x, y) assert_helper(x, y, __FILE__, __LINE__)
#define kassert_eq(x, y) assert_eq_helper(x, y, "", __FILE__, __LINE__)
#define kassert_eq_msg(x, y, z) assert_eq_helper(x, y, z, __FILE__, __LINE__)

template <typename T>
auto assert_helper(T val, const std::string &error_msg = "",
                   const std::string &file_name = "",
                   uint32_t line_number         = 0) {
    if (!val) {
        std::ostringstream oss;
        if (!file_name.empty() and line_number != 0) {
            oss << "In file: " << file_name << "\n"
                << "On line: " << line_number << "\n";
        }
        oss << "Expected true\nReceived `" << val << "`";
        if (!error_msg.empty()) { oss << "\n" << error_msg; }
        oss << "\n---";
        eprintln(oss.str());
    }
}

template <typename T, typename U>
auto assert_eq_helper(T lhs, U rhs, const std::string &err_msg = "",
                      const std::string_view file_name = "",
                      uint32_t line_number             = 0) {
    if (lhs != rhs) {
        std::ostringstream oss;
        // clang-format off
        if (!file_name.empty() and line_number != 0) {
            oss << "In file: " << file_name << "\n"
                << "On line: " << line_number << "\n";
        }
        oss << "Expected `"    << rhs << "`\n"
            << "Received `" << lhs << "`";
        if (!err_msg.empty()) oss << "\n" << err_msg;
        oss << "\n---";
        // clang-format on
        eprintln(oss.str());
    }
}

/// MISC
auto re_search(const std::string &input, const std::string &re) -> bool;

auto lines_from_file(const std::string &input) -> std::vector<std::string>;

} // namespace khelper
