/*
 * csv.hpp
 *
 * Licensed under the MIT License
 *
 * Copyright (c) 2021 MarkusJx
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef MARKUSJX_CSV_HPP
#define MARKUSJX_CSV_HPP

#include <vector>
#include <sstream>
#include <string>
#include <regex>

#if defined(_MSVC_LANG) || defined(__cplusplus)
#   if (defined(_MSVC_LANG) && _MSVC_LANG > 201703L) || __cplusplus > 201703L // C++20
#       define CSV_REQUIRES(type, ...) template<class = type> requires ::markusjx::util::is_any_of_v<type, __VA_ARGS__>
#   endif
#   if (defined(_MSVC_LANG) && _MSVC_LANG > 201402L) || __cplusplus > 201402L // C++17
#       ifndef CSV_REQUIRES
#           define CSV_REQUIRES(type, ...) template<class U = type, typename = std::enable_if_t<::markusjx::util::is_any_of_v<U, __VA_ARGS__>, int>>
#       endif //CSV_REQUIRES
#       if defined(__APPLE__) && defined(__clang__)
#           define CSV_NODISCARD __attribute__ ((warn_unused_result))
#           warning Compiling on apple with clang may lead to issues. Consider compiling using gcc
#       else
#           define CSV_NODISCARD [[nodiscard]]
#       endif
#   else
#       error Use of C++14 and lower is not (yet) supported. Please use C++17 or C++20 instead
#   endif
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#   define CSV_WINDOWS
#   undef CSV_UNIX
#pragma message("Building on windows")
#elif defined(__LINUX__) || defined(__APPLE__) || defined (__CYGWIN__) || defined(__linux__) || defined(__FreeBSD__) || \
        defined(unix) || defined(__unix) || defined(__unix__)
#   define CSV_UNIX
#   undef CSV_WINDOWS
#pragma message("Building on unix")
#endif

#ifndef CSV_NODISCARD
#   define CSV_NODISCARD
#endif

#ifndef CSV_REQUIRES
#   define CSV_REQUIRES(type, ...)
#endif //CSV_REQUIRES

#define CSV_CHECK_T_SUPPORTED static_assert(::markusjx::util::is_u8_string_v<T> || ::markusjx::util::is_u16_string_v<T>,\
                                            "T must be one of: [std::string, std::wstring]");

namespace markusjx {
    namespace util {
        template<class T, class... Types>
        inline constexpr bool is_any_of_v = std::disjunction_v<std::is_same<T, Types>...>;

        template<class T>
        inline constexpr bool is_u8_string_v = is_any_of_v<T, std::string>;

        template<class T>
        inline constexpr bool is_u16_string_v = is_any_of_v<T, std::wstring>;

        template<class T>
        inline std::vector<T> splitString(const T &str, char delimiter) {
            std::vector<T> tokens;
            size_t prev = 0, pos;
            do {
                pos = str.find(delimiter, prev);
                if (pos == T::npos) pos = str.length();
                T token = str.substr(prev, pos - prev);
                tokens.push_back(token);
                prev = pos + 1;
            } while (pos < str.length() && prev < str.length());
            return tokens;
        }

        template<class T, class S>
        inline T stringToNumberAlt(const S &str, T(*conversionFunc)(const S &, size_t *)) {
            size_t idx = 0;
            T res = conversionFunc(str, &idx);

            if (idx != 0 && idx != str.size()) {
                throw std::runtime_error("Could not fully convert the value");
            } else {
                return res;
            }
        }

        template<class T, class S>
        inline T stringToNumber(const S &str, T(*conversionFunc)(const S &, size_t *, int)) {
            size_t idx = 0;
            T res = conversionFunc(str, &idx, 10);

            if (idx == 0) {
                throw std::runtime_error("Could not convert the value");
            } else {
                return res;
            }
        }

        inline std::string wstring_to_string(const std::wstring &in) {
            std::string out(in.size() + 1, ' ');

#ifdef CSV_WINDOWS
            size_t outSize;

            errno_t err = wcstombs_s(&outSize, out.data(), out.size(), in.c_str(), in.size());
            if (err) {
                throw std::runtime_error("Could not convert the string");
            }
#elif defined(CSV_UNIX)
            size_t written = wcstombs(out.data(), in.c_str(), in.size());
            if (written == static_cast<size_t>(-1)) {
                throw std::runtime_error("Could not convert the string");
            }
#endif // WINODWS OR UNIX

            out.resize(in.size());
            return out;
        }

        inline std::wstring string_to_wstring(const std::string &in) {
            std::wstring out(in.size() + 1, L' ');

#ifdef CSV_WINDOWS
            size_t outSize;
            errno_t err = mbstowcs_s(&outSize, (wchar_t *) out.data(), out.size(), in.c_str(), in.size());
            if (err) {
                throw std::runtime_error("Could not create the string");
            }
#elif defined(CSV_UNIX)
            size_t written = mbstowcs(out.data(), in.c_str(), in.size());
            if (written == static_cast<size_t>(-1)) {
                throw std::runtime_error("Could not convert the string");
            }
#endif // WINDOWS OR UNIX

            out.resize(in.size());
            return out;
        }
    }

    template<class T>
    class csvrowcolumn {
    public:
        CSV_CHECK_T_SUPPORTED

        static csvrowcolumn<T> parse(const T &value) {
            csvrowcolumn<T> col(nullptr);
            col.value = value;

            return col;
        }

        csvrowcolumn(std::nullptr_t) : value() {}

        csvrowcolumn(const T &val) : value('"' + val + '"') {}

        CSV_REQUIRES(T, std::wstring)
        csvrowcolumn(const std::string &val) : value(L'"' + util::string_to_wstring(val) + L'"') {}

        csvrowcolumn(char val) : value() {
            value += '"';
            value += val;
            value += '"';
        }

        csvrowcolumn(const char *val) {
            if constexpr (util::is_u8_string_v<T>) {
                value = '"' + T(val) + '"';
            } else if constexpr (util::is_u16_string_v<T>) {
                value = L'"' + util::string_to_wstring(val) + L'"';
            }
        }

        CSV_REQUIRES(T, std::wstring)
        csvrowcolumn(wchar_t val) : value() {
            value += L'"';
            value += val;
            value += L'"';
        }

        CSV_REQUIRES(T, std::wstring)
        csvrowcolumn(const wchar_t *val) : value(L'"' + T(val) + L'"') {}

        csvrowcolumn(bool val) {
            if constexpr (util::is_u8_string_v<T>) {
                value = T(val ? "true" : "false");
            } else if constexpr (util::is_u16_string_v<T>) {
                value = T(val ? L"true" : L"false");
            }
        }

        template<class U>
        csvrowcolumn(const U &val) {
            if constexpr (util::is_u8_string_v<T>) {
                value = std::to_string(val);
            } else if constexpr (util::is_u16_string_v<T>) {
                value = std::to_wstring(val);
            }
        }

        csvrowcolumn<T> &operator=(const csvrowcolumn<T> &) = default;

        csvrowcolumn<T> &operator=(std::nullptr_t) {
            if constexpr (util::is_u8_string_v<T>) {
                value = "";
            } else if constexpr (util::is_u8_string_v<T>) {
                value = L"";
            }

            return *this;
        }

        csvrowcolumn<T> &operator=(const T &val) {
            value = T();
            value += '"';
            value += val;
            value += '"';
            return *this;
        }

        csvrowcolumn<T> &operator=(const char *val) {
            if constexpr (util::is_u8_string_v<T>) {
                value = '"' + T(val) + '"';
            } else if constexpr (util::is_u16_string_v<T>) {
                value = L'"' + util::string_to_wstring(val) + L'"';
            }
            return *this;
        }

        CSV_REQUIRES(T, std::wstring)
        csvrowcolumn<T> &operator=(const wchar_t *val) {
            value = L'"' + T(val) + L'"';
            return *this;
        }

        csvrowcolumn<T> &operator=(bool val) {
            if constexpr (util::is_u8_string_v<T>) {
                value = std::string(val ? "true" : "false");
            } else if constexpr (util::is_u16_string_v<T>) {
                value = std::wstring(val ? L"true" : L"false");
            }

            return *this;
        }

        csvrowcolumn<T> &operator=(char val) {
            value = T();
            value += '"';
            value += val;
            value += '"';
            return *this;
        }

        CSV_REQUIRES(T, std::wstring)
        csvrowcolumn<T> &operator=(wchar_t val) {
            value = T();
            value += L'"';
            value += val;
            value += L'"';
            return *this;
        }

        CSV_REQUIRES(T, std::wstring)
        csvrowcolumn &operator=(const std::string &val) {
            value = L'"' + util::string_to_wstring(val) + L'"';
            return *this;
        }

        template<class U>
        csvrowcolumn &operator=(const U &val) {
            if constexpr (util::is_u8_string_v<T>) {
                value = std::to_string(val);
            } else if constexpr (util::is_u16_string_v<T>) {
                value = std::to_wstring(val);
            }

            return *this;
        }

        CSV_NODISCARD T rawValue() const {
            return value;
        }

        CSV_NODISCARD operator T() const {
            if (value[0] == '"') {
                return value.substr(1, value.size() - 2);
            } else {
                throw std::runtime_error("The stored value is not a string");
            }
        }

        CSV_REQUIRES(T, std::string)
        CSV_NODISCARD operator char() const {
            T val = this->operator T();
            if (val.size() == 1) {
                return val[0];
            } else {
                throw std::runtime_error("The value is not a character");
            }
        }

        CSV_REQUIRES(T, std::wstring)
        CSV_NODISCARD operator wchar_t() const {
            T val = this->operator T();
            if (val.size() == 1) {
                return val[0];
            } else {
                throw std::runtime_error("The value is not a character");
            }
        }

        CSV_NODISCARD operator int() const {
            return util::stringToNumber<int>(value, std::stoi);
        }

        CSV_NODISCARD operator long() const {
            return util::stringToNumber<long>(value, std::stol);
        }

        CSV_NODISCARD operator unsigned long() const {
            return util::stringToNumber<unsigned long>(value, std::stoul);
        }

        CSV_NODISCARD operator long long() const {
            return util::stringToNumber<long long>(value, std::stoll);
        }

        CSV_NODISCARD operator unsigned long long() const {
            return util::stringToNumber<unsigned long long>(value, std::stoull);
        }

        CSV_NODISCARD operator double() const {
            return util::stringToNumberAlt<double>(value, std::stod);
        }

        CSV_NODISCARD operator long double() const {
            return util::stringToNumberAlt<long double>(value, std::stold);
        }

        CSV_NODISCARD operator float() const {
            return util::stringToNumberAlt<float>(value, std::stof);
        }

        CSV_NODISCARD operator bool() const {
            if constexpr (util::is_u8_string_v<T>) {
                return op_bool_impl(value, "true", "false");
            } else if constexpr (util::is_u16_string_v<T>) {
                return op_bool_impl(value, L"true", L"false");
            }
        }

        template<class U>
        CSV_NODISCARD U as() const {
            return this->operator U();
        }

        template<class U>
        CSV_NODISCARD bool operator==(const U &val) const {
            return this->as<U>() == val;
        }

        template<class U>
        CSV_NODISCARD bool operator!=(const U &val) const {
            return this->as<U>() != val;
        }

        CSV_NODISCARD bool operator==(const csvrowcolumn<T> &other) const {
            if (this == &other) {
                return true;
            } else {
                return this->rawValue() == other.rawValue();
            }
        }

        CSV_NODISCARD bool operator!=(const csvrowcolumn<T> &other) const {
            if (this == &other) {
                return false;
            } else {
                return this->rawValue() != other.rawValue();
            }
        }

        CSV_NODISCARD bool operator<(const csvrowcolumn<T> &other) const {
            if (this->isFloatingPoint() && other.isFloatingPoint()) {
                return this->as<long double>() < other.as<long double>();
            } else if (this->isFloatingPoint() && other.isDecimal()) {
                return this->as<long double>() < other.as<long long>();
            } else if (this->isDecimal() && other.isFloatingPoint()) {
                return this->as<long long>() < other.as<long double>();
            } else if (this->isDecimal() && other.isDecimal()) {
                return this->as<long long>() < other.as<long long>();
            } else {
                return this->value < other.value;
            }
        }

        template<class U>
        CSV_NODISCARD bool operator<(const U &val) const {
            return this->as<U>() < val;
        }

        CSV_NODISCARD bool operator<=(const csvrowcolumn<T> &other) const {
            if (this->isFloatingPoint() && other.isFloatingPoint()) {
                return this->as<long double>() <= other.as<long double>();
            } else if (this->isFloatingPoint() && other.isDecimal()) {
                return this->as<long double>() <= other.as<long long>();
            } else if (this->isDecimal() && other.isFloatingPoint()) {
                return this->as<long long>() <= other.as<long double>();
            } else if (this->isDecimal() && other.isDecimal()) {
                return this->as<long long>() <= other.as<long long>();
            } else {
                return this->value <= other.value;
            }
        }

        template<class U>
        CSV_NODISCARD bool operator<=(const U &val) const {
            return this->as<U>() <= val;
        }

        CSV_NODISCARD bool operator>(const csvrowcolumn<T> &other) const {
            if (this->isFloatingPoint() && other.isFloatingPoint()) {
                return this->as<long double>() > other.as<long double>();
            } else if (this->isFloatingPoint() && other.isDecimal()) {
                return this->as<long double>() > other.as<long long>();
            } else if (this->isDecimal() && other.isFloatingPoint()) {
                return this->as<long long>() > other.as<long double>();
            } else if (this->isDecimal() && other.isDecimal()) {
                return this->as<long long>() > other.as<long long>();
            } else {
                return this->value > other.value;
            }
        }

        template<class U>
        CSV_NODISCARD bool operator>(const U &val) const {
            return this->as<U>() > val;
        }

        CSV_NODISCARD bool operator>=(const csvrowcolumn<T> &other) const {
            if (this->isFloatingPoint() && other.isFloatingPoint()) {
                return this->as<long double>() >= other.as<long double>();
            } else if (this->isFloatingPoint() && other.isDecimal()) {
                return this->as<long double>() >= other.as<long long>();
            } else if (this->isDecimal() && other.isFloatingPoint()) {
                return this->as<long long>() >= other.as<long double>();
            } else if (this->isDecimal() && other.isDecimal()) {
                return this->as<long long>() >= other.as<long long>();
            } else {
                return this->value >= other.value;
            }
        }

        template<class U>
        CSV_NODISCARD bool operator>=(const U &val) const {
            return this->as<U>() >= val;
        }

        const csvrowcolumn<T> operator++(int) {
            if (isDecimal()) {
                this->operator=(this->as<long long>() + 1);
            } else {
                this->operator=(this->as<long double>() + 1.0);
            }
            return *this;
        }

        const csvrowcolumn<T> operator--(int) {
            if (isDecimal()) {
                this->operator=(this->as<long long>() - 1);
            } else {
                this->operator=(this->as<long double>() - 1.0);
            }
            return *this;
        }

        CSV_NODISCARD csvrowcolumn<T> operator+(const csvrowcolumn<T> &val) const {
            if (this->isFloatingPoint() || val.isFloatingPoint()) {
                return csvrowcolumn(this->as<long double>() + val.as<long double>());
            } else if (this->isNumber() && val.isNumber()) {
                return csvrowcolumn(this->as<long long>() + val.as<long long>());
            } else {
                throw std::runtime_error("The value is not a number");
            }
        }

        template<class U>
        CSV_NODISCARD csvrowcolumn<T> operator+(const U &val) const {
            return csvrowcolumn(this->as<U>() + val);
        }

        template<class U>
        csvrowcolumn<T> &operator+=(const U &val) {
            return this->operator=(this->operator+(val));
        }

        CSV_NODISCARD csvrowcolumn<T> operator-(const csvrowcolumn<T> &val) const {
            if (this->isFloatingPoint() || val.isFloatingPoint()) {
                return csvrowcolumn(this->as<long double>() - val.as<long double>());
            } else if (this->isNumber() && val.isNumber()) {
                return csvrowcolumn(this->as<long long>() - val.as<long long>());
            } else {
                throw std::runtime_error("The value is not a number");
            }
        }

        template<class U>
        CSV_NODISCARD csvrowcolumn<T> operator-(const U &val) const {
            return csvrowcolumn(this->as<U>() - val);
        }

        template<class U>
        csvrowcolumn<T> &operator-=(const U &val) {
            return this->operator=(this->operator-(val));
        }

        CSV_NODISCARD csvrowcolumn<T> operator*(const csvrowcolumn<T> &val) const {
            if (this->isFloatingPoint() || val.isFloatingPoint()) {
                return csvrowcolumn(this->as<long double>() * val.as<long double>());
            } else if (this->isNumber() && val.isNumber()) {
                return csvrowcolumn(this->as<long long>() * val.as<long long>());
            } else {
                throw std::runtime_error("The value is not a number");
            }
        }

        template<class U>
        CSV_NODISCARD csvrowcolumn<T> operator*(const U &val) const {
            return csvrowcolumn(this->as<U>() * val);
        }

        template<class U>
        csvrowcolumn<T> &operator*=(const U &val) {
            return this->operator=(this->operator*(val));
        }

        CSV_NODISCARD csvrowcolumn<T> operator/(const csvrowcolumn<T> &val) const {
            if (this->isFloatingPoint() || val.isFloatingPoint()) {
                return csvrowcolumn(this->as<long double>() / val.as<long double>());
            } else if (this->isNumber() && val.isNumber()) {
                return csvrowcolumn(this->as<long long>() / val.as<long long>());
            } else {
                throw std::runtime_error("The value is not a number");
            }
        }

        template<class U>
        CSV_NODISCARD csvrowcolumn<T> operator/(const U &val) const {
            return csvrowcolumn(this->as<U>() / val);
        }

        template<class U>
        csvrowcolumn<T> &operator/=(const U &val) {
            return this->operator=(this->operator/(val));
        }

        CSV_NODISCARD bool empty() const {
            return value.empty();
        }

        CSV_NODISCARD bool isNumber() const {
            const static std::regex number_regex("^-?[0-9]+(\\.[0-9]+)?$");
            return std::regex_match(value, number_regex);
        }

        CSV_NODISCARD bool isDecimal() const {
            const static std::regex decimal_regex("^-?[0-9]+$");
            return std::regex_match(value, decimal_regex);
        }

        CSV_NODISCARD bool isFloatingPoint() const {
            const static std::regex float_regex("^-?[0-9]+\\.[0-9]+$");
            return std::regex_match(value, float_regex);
        }

    private:
        template<class U>
        CSV_NODISCARD bool op_bool_impl(const T &val, U true_val, U false_val) const {
            if (val == true_val) {
                return true;
            } else if (val == false_val) {
                return false;
            } else {
                throw std::runtime_error("Could not convert the value");
            }
        }

        T value;
    };

    template<class T>
    class csvrow {
    public:
        CSV_CHECK_T_SUPPORTED

        static csvrow<T> parse(const T &value, const char separator) {
            csvrow row(nullptr);
            for (const T &col : util::splitString(value, separator)) {
                row << csvrowcolumn<T>::parse(col);
            }

            return row;
        }

        explicit csvrow(std::nullptr_t) : columns() {}

        csvrow(const std::vector<csvrowcolumn<T>> &data) : columns(data) {}

        template<class U>
        csvrow(const std::vector<U> &data) : columns(data.size()) {
            for (size_t i = 0; i < data.size(); i++) {
                columns[i] = csvrowcolumn<T>(data[i]);
            }
        }

        csvrow(const std::initializer_list<csvrowcolumn<T>> &data) : columns(data) {}

        template<class U>
        csvrow(const std::initializer_list<U> &list) : columns() {
            columns.reserve(list.size());
            for (const U &val: list) {
                columns.emplace_back(val);
            }
        }

        csvrow<T> &operator=(const csvrow<T> &) = default;

        csvrow<T> &operator=(const std::vector<csvrowcolumn<T>> &data) {
            this->columns = data;
            return *this;
        }

        template<class U>
        csvrow<T> &operator=(const std::vector<U> &data) {
            for (size_t i = 0; i < data.size(); i++) {
                this->at(i).operator=(data.at(i));
            }

            return *this;
        }

        csvrow<T> &operator=(const std::initializer_list<csvrowcolumn<T>> &data) {
            this->columns = std::vector<csvrowcolumn<T>>(data);
            return *this;
        }

        template<class U>
        csvrow<T> &operator=(const std::initializer_list<U> &data) {
            for (size_t i = 0; i < data.size(); i++) {
                this->at(i).operator=(data[i]);
            }

            return *this;
        }

        csvrowcolumn<T> &at(size_t index) {
            if (index >= columns.size()) {
                for (size_t i = columns.size(); i <= index; i++) {
                    columns.emplace_back(nullptr);
                }
            }

            return this->columns.at(index);
        }

        const csvrowcolumn<T> &at(size_t index) const {
            return this->columns.at(index);
        }

        csvrowcolumn<T> &operator[](size_t columnIndex) {
            return this->at(columnIndex);
        }

        const csvrowcolumn<T> &operator[](size_t columnIndex) const {
            return this->at(columnIndex);
        }

        csvrow &operator<<(const csvrowcolumn<T> &col) {
            columns.push_back(col);
            return *this;
        }

        CSV_NODISCARD bool operator==(const csvrow<T> &other) const {
            if (this == &other) {
                return true;
            }

            if (this->size() == other.size()) {
                for (size_t i = 0; i < this->size(); i++) {
                    if (this->at(i) != other.at(i)) {
                        return false;
                    }
                }

                return true;
            } else {
                return false;
            }
        }

        CSV_NODISCARD bool operator!=(const csvrow<T> &other) const {
            return !this->operator==(other);
        }

        csvrowcolumn<T> &get_next() {
            return columns.emplace_back(nullptr);
        }

        typename std::vector<csvrowcolumn<T>>::iterator begin() noexcept {
            return columns.begin();
        }

        CSV_NODISCARD typename std::vector<csvrowcolumn<T>>::const_iterator begin() const noexcept {
            return columns.cbegin();
        }

        typename std::vector<csvrowcolumn<T>>::iterator end() noexcept {
            return columns.end();
        }

        CSV_NODISCARD typename std::vector<csvrowcolumn<T>>::const_iterator end() const noexcept {
            return columns.cend();
        }

        CSV_NODISCARD size_t size() const noexcept {
            return columns.size();
        }

        CSV_NODISCARD bool empty() const noexcept {
            return columns.empty();
        }

        void clear() noexcept {
            columns.clear();
        }

    private:
        std::vector<csvrowcolumn<T>> columns;
    };

    template<class T>
    class basic_csv {
    public:
        CSV_CHECK_T_SUPPORTED

        static basic_csv<T> parse(const T &value, const char separator = ';') {
            basic_csv<T> csv(separator);
            csv.parseImpl(value);

            return csv;
        }

        static basic_csv<T> &endl(basic_csv<T> &c) {
            c.rows.emplace_back(nullptr);
            return c;
        }

        explicit basic_csv(char separator = ';') : rows(), separator(separator) {}

        basic_csv(std::nullptr_t, char separator = ';') : rows(), separator(separator) {}

        basic_csv(const T &value, char separator = ';') : rows(), separator(separator) {
            this->parseImpl(value);
        }

        CSV_REQUIRES(T, std::wstring)
        basic_csv(const std::string &value, char separator = ';') : rows(), separator(separator) {
            this->parseImpl(util::string_to_wstring(value));
        }

        basic_csv(const char *value, char separator = ';') : rows(), separator(separator) {
            if constexpr (util::is_u8_string_v<T>) {
                this->parseImpl(T(value));
            } else if constexpr (util::is_u16_string_v<T>) {
                this->parseImpl(util::string_to_wstring(value));
            }
        }

        CSV_REQUIRES(T, std::wstring)
        basic_csv(const wchar_t *value, char separator = ';') : rows(), separator(separator) {
            this->parseImpl(T(value));
        }

        basic_csv(const std::vector<csvrow<T>> &data, char separator = ';') : rows(data), separator(separator) {}

        basic_csv(const std::vector<csvrowcolumn<T>> &data, char separator = ';') : rows({csvrow<T>(data)}),
                                                                                    separator(separator) {}

        basic_csv(const std::initializer_list<csvrow<T>> &data, char separator = ';') : rows(data),
                                                                                        separator(separator) {}

        basic_csv(const std::initializer_list<csvrowcolumn<T>> &data, char separator = ';') : rows({csvrow<T>(data)}),
                                                                                              separator(separator) {}

        csvrow<T> &at(size_t index) {
            if (index >= this->rows.size()) {
                for (size_t i = rows.size(); i <= index; i++) {
                    rows.emplace_back(nullptr);
                }
            }

            return rows[index];
        }

        const csvrow<T> &at(size_t index) const {
            return rows[index];
        }

        csvrow<T> &operator[](size_t index) {
            return this->at(index);
        }

        const csvrow<T> &operator[](size_t index) const {
            return this->at(index);
        }

        basic_csv<T> &operator<<(const basic_csv<T> &other) {
            for (const csvrow<T> &row : other) {
                rows.push_back(row);
            }

            return *this;
        }

        basic_csv<T> &operator<<(const csvrowcolumn<T> &col) {
            get_current() << col;
            return *this;
        }

        basic_csv<T> &operator<<(const csvrow<T> &row) {
            this->rows.push_back(row);
            return *this;
        }

        template<class U>
        basic_csv<T> &operator<<(const U &val) {
            get_current().get_next() = val;
            return *this;
        }

        CSV_NODISCARD bool operator==(const basic_csv<T> &other) const {
            if (this == &other) {
                return true;
            }

            if (this->size() == other.size()) {
                for (size_t i = 0; i < this->size(); i++) {
                    if (this->at(i) != other.at(i)) {
                        return false;
                    }
                }

                return true;
            } else {
                return false;
            }
        }

        CSV_NODISCARD bool operator!=(const basic_csv<T> &other) const {
            return !this->operator==(other);
        }

        basic_csv<T> &push(const std::vector<csvrowcolumn<T>> &data) {
            for (const csvrowcolumn<T> &col : data) {
                this->template operator<<(col);
            }
            return *this;
        }

        basic_csv<T> &push(const std::initializer_list<csvrowcolumn<T>> &data) {
            for (const csvrowcolumn<T> &col : data) {
                this->template operator<<(col);
            }
            return *this;
        }

        basic_csv<T> &push(const std::vector<csvrow<T>> &data) {
            for (const csvrow<T> &col : data) {
                this->push(col);
            }
            return *this;
        }

        basic_csv<T> &push(const std::initializer_list<csvrow<T>> &data) {
            for (const csvrow<T> &col : data) {
                this->push(col);
            }
            return *this;
        }

        template<class U>
        basic_csv<T> &push(const U &val) {
            return this->template operator<<(val);
        }

        template<class U>
        basic_csv<T> &operator+=(const U &val) {
            return this->push(val);
        }

        template<class U>
        CSV_NODISCARD basic_csv<T> operator+(const U &val) const {
            basic_csv<T> res(*this);
            res.push(val);

            return res;
        }

        basic_csv<T> &operator<<(basic_csv<T> &(*val)(basic_csv<T> &)) {
            return val(*this);
        }

        basic_csv<T> &endline() {
            rows.emplace_back(nullptr);
            return *this;
        }

        CSV_NODISCARD T to_string() const {
            if constexpr (util::is_u8_string_v<T>) {
                return to_string_impl<std::stringstream>();
            } else if constexpr (util::is_u16_string_v<T>) {
                return to_string_impl<std::wstringstream>();
            }
        }

        CSV_NODISCARD std::string to_u8string() const {
            if constexpr (util::is_u8_string_v<T>) {
                return this->to_string();
            } else if constexpr (util::is_u16_string_v<T>) {
                return util::wstring_to_string(this->to_string());
            }
        }

        CSV_NODISCARD std::wstring to_u16string() const {
            if constexpr (util::is_u8_string_v<T>) {
                return util::string_to_wstring(this->to_string());
            } else if constexpr (util::is_u16_string_v<T>) {
                return this->to_string();
            }
        }

        typename std::vector<csvrow<T>>::iterator begin() noexcept {
            return rows.begin();
        }

        CSV_NODISCARD typename std::vector<csvrow<T>>::const_iterator begin() const noexcept {
            return rows.begin();
        }

        typename std::vector<csvrow<T>>::iterator end() noexcept {
            return rows.end();
        }

        CSV_NODISCARD typename std::vector<csvrow<T>>::const_iterator end() const noexcept {
            return rows.end();
        }

        CSV_NODISCARD size_t size() const {
            return rows.size();
        }

        CSV_NODISCARD bool empty() const {
            return rows.empty();
        }

        void clear() noexcept {
            rows.clear();
        }

        CSV_NODISCARD size_t numElements() const {
            size_t size = 0;
            for (const csvrow<T> &row : rows) {
                size += row.size();
            }

            return size;
        }

        friend std::ostream &operator<<(std::ostream &ostream, const basic_csv<T> &csv) {
            if constexpr (util::is_u8_string_v<T>) {
                ostream << csv.to_string();
            } else if constexpr (util::is_u16_string_v<T>) {
                ostream << util::wstring_to_string(csv.to_string());
            }

            return ostream;
        }

        friend std::wostream &operator<<(std::wostream &ostream, const basic_csv<T> &csv) {
            if constexpr (util::is_u16_string_v<T>) {
                ostream << csv.to_string();
            } else if constexpr (util::is_u8_string_v<T>) {
                ostream << util::string_to_wstring(csv.to_string());
            }

            return ostream;
        }

        friend std::istream &operator>>(std::istream &istream, basic_csv<T> &csv) {
            std::string res(std::istreambuf_iterator<char>(istream), {});

            if constexpr (util::is_u8_string_v<T>) {
                csv << basic_csv::parse(res, csv.separator);
            } else if constexpr (util::is_u16_string_v<T>) {
                csv << basic_csv::parse(util::string_to_wstring(res), csv.separator);
            }

            return istream;
        }

        friend std::wistream &operator>>(std::wistream &istream, basic_csv<T> &csv) {
            std::wstring res(std::istreambuf_iterator<wchar_t>(istream), {});

            if constexpr (util::is_u8_string_v<T>) {
                csv << basic_csv::parse(util::wstring_to_string(res), csv.separator);
            } else if constexpr (util::is_u16_string_v<T>) {
                csv << basic_csv::parse(res, csv.separator);
            }

            return istream;
        }

    private:
        csvrow<T> &get_current() {
            if (rows.empty()) {
                rows.emplace_back(nullptr);
            }

            return rows[rows.size() - 1];
        }

        template<class U>
        CSV_NODISCARD T to_string_impl() const {
            U ss;
            for (size_t i = 0; i < rows.size(); i++) {
                for (const csvrowcolumn<T> &col : rows[i]) {
                    ss << col.rawValue() << separator;
                }

                if (i < rows.size() - 1) {
                    ss << std::endl;
                }
            }

            return ss.str();
        }

        void parseImpl(const T &value) {
            // Split the string by newlines and
            // push the lines into the row vector
            for (const T &row : util::splitString(value, '\n')) {
                rows.push_back(csvrow<T>::parse(row, separator));
            }

            // If value ends with a new line, insert a
            // new line into this objects row array
            if (value[value.size() - 1] == '\n') {
                rows.emplace_back(nullptr);
            }
        }

        std::vector<csvrow<T>> rows;
        char separator;
    };

    using csv = basic_csv<std::string>;
    using w_csv = basic_csv<std::wstring>;
}

#undef CSV_NODISCARD
#undef CSV_CHECK_T_SUPPORTED
#undef CSV_WINDOWS
#undef CSV_UNIX
#undef CSV_REQUIRES

#endif //MARKUSJX_CSV_HPP
