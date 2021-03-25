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
#include <map>

#if defined(_MSVC_LANG) || defined(__cplusplus)
#   if (defined(_MSVC_LANG) && _MSVC_LANG > 201703L) || __cplusplus > 201703L // C++20
#       define CSV_REQUIRES(type, ...) template<class = type> requires ::markusjx::util::is_any_of_v<type, __VA_ARGS__>
#   endif
#   if (defined(_MSVC_LANG) && _MSVC_LANG > 201402L) || __cplusplus > 201402L // C++17
#       ifndef CSV_REQUIRES
//          Use SFINAE to disable functions
#           define CSV_REQUIRES(type, ...) template<class _SFINAE_PARAM_ = type, class = typename std::enable_if_t<::markusjx::util::is_any_of_v<_SFINAE_PARAM_, __VA_ARGS__>, int>>
#       endif //CSV_REQUIRES
//      This looks awful, but it works
#       define CSV_ENABLE_IF(...) class = typename std::enable_if_t<__VA_ARGS__, int>
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
#elif defined(__LINUX__) || defined(__APPLE__) || defined (__CYGWIN__) || defined(__linux__) || defined(__FreeBSD__) || \
        defined(unix) || defined(__unix) || defined(__unix__)
#   define CSV_UNIX
#   undef CSV_WINDOWS
#endif

#ifndef CSV_NODISCARD
#   define CSV_NODISCARD
#endif

#ifndef CSV_REQUIRES
#   define CSV_REQUIRES(type, ...)
#endif //CSV_REQUIRES

// Check if <T> is supported
#define CSV_CHECK_T_SUPPORTED static_assert(::markusjx::util::is_u8_string_v<T> || ::markusjx::util::is_u16_string_v<T>,\
                                            "T must be one of: [std::string, std::wstring]");

namespace markusjx {
    /**
     * A utility namespace
     */
    namespace util {
        /**
         * Check if T is any of Types
         *
         * @tparam T the type to check
         * @tparam Types the types to check against
         */
        template<class T, class... Types>
        inline constexpr bool is_any_of_v = std::disjunction_v<std::is_same<T, Types>...>;

        /**
         * Check if T is a utf-8 string
         *
         * @tparam T the type to check
         */
        template<class T>
        inline constexpr bool is_u8_string_v = is_any_of_v<T, std::string>;

        /**
         * Check if T is a utf-16 string
         *
         * @tparam T the type to check
         */
        template<class T>
        inline constexpr bool is_u16_string_v = is_any_of_v<T, std::wstring>;

        /**
         * Split a string.
         * Source: https://stackoverflow.com/a/37454181
         *
         * @tparam T the string type
         * @param str the string to split
         * @param delimiter the delimiter
         * @return the string elements
         */
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

        /**
         * Convert a string to a number. Alternative version
         *
         * @tparam T the number type
         * @tparam S the string type
         * @param str the string to convert
         * @param conversionFunc the conversion function
         * @return the converted number
         */
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

        /**
         * Convert a string to a number
         *
         * @tparam T the number type
         * @tparam S the string type
         * @param str the string to convert
         * @param conversionFunc the conversion function
         * @return the converted number
         */
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

        /**
         * Convert a std::wstring to a std::string
         *
         * @param in the wide string to convert
         * @return the converted string
         */
        inline std::string wstring_to_string(const std::wstring &in) {
            // Create the result string
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

        /**
         * Convert a std::string to a std::wstring
         *
         * @param in the string to convert
         * @return the converted wide string
         */
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

        /**
         * Get a string as another string type
         *
         * @tparam T the string type to convert to
         * @tparam U the type of the string to convert
         * @param str the string to convert
         * @return the converted string
         */
        template<class T, class U, CSV_ENABLE_IF(
                (is_u8_string_v<T> || is_u16_string_v<T>) && (is_u8_string_v<U> || is_u16_string_v<U>)) >
        inline T string_as(const U &str) {
            static_assert(is_u8_string_v<T> || is_u16_string_v<T>);
            static_assert(is_u8_string_v<U> || is_u16_string_v<U>);

            if constexpr ((is_u8_string_v<T> && is_u8_string_v<U>) || (is_u16_string_v<T> && is_u16_string_v<U>)) {
                return str;
            } else if constexpr (is_u8_string_v<T> && is_u16_string_v<U>) {
                return wstring_to_string(str);
            } else if constexpr (is_u16_string_v<T> && is_u8_string_v<U>) {
                return string_to_wstring(str);
            }
        }
    }

    /**
     * A csv column
     *
     * @tparam T the string type. Must be a std::string or std::wstring
     */
    template<class T>
    class csvrowcolumn {
    public:
        CSV_CHECK_T_SUPPORTED

        /**
         * Parse a column value
         *
         * @param value the string value to parse
         * @return the parsed column
         */
        static csvrowcolumn<T> parse(const T &value) {
            csvrowcolumn<T> col(nullptr);
            col.value = value;

            return col;
        }

        /**
         * Create an empty column
         */
        csvrowcolumn(std::nullptr_t) : value() {}

        /**
         * Create a column from a string value
         *
         * @param val the value
         */
        csvrowcolumn(const T &val) : value('"' + val + '"') {}

        /**
         * Create a column from a string value. Only available if T = std::wstring
         *
         * @param val the string to convert
         */
        CSV_REQUIRES(T, std::wstring)
        csvrowcolumn(const std::string &val) : value(L'"' + util::string_to_wstring(val) + L'"') {}

        /**
         * Create a column from a character
         *
         * @param val the character to use
         */
        csvrowcolumn(char val) : value() {
            value += '"';
            value += val;
            value += '"';
        }

        /**
         * Create a column from a char array
         *
         * @param val the char array
         */
        csvrowcolumn(const char *val) {
            if constexpr (util::is_u8_string_v<T>) {
                value = '"' + T(val) + '"';
            } else if constexpr (util::is_u16_string_v<T>) {
                value = L'"' + util::string_to_wstring(val) + L'"';
            }
        }

        /**
         * Create a column from a wide character
         *
         * @param val the char to use
         */
        CSV_REQUIRES(T, std::wstring)
        csvrowcolumn(wchar_t val) : value() {
            value += L'"';
            value += val;
            value += L'"';
        }

        /**
         * Create a column from a wide char array
         *
         * @param val the char array
         */
        CSV_REQUIRES(T, std::wstring)
        csvrowcolumn(const wchar_t *val) : value(L'"' + T(val) + L'"') {}

        /**
         * Create a column from a boolean
         *
         * @param val the bool
         */
        csvrowcolumn(bool val) {
            if constexpr (util::is_u8_string_v<T>) {
                value = T(val ? "true" : "false");
            } else if constexpr (util::is_u16_string_v<T>) {
                value = T(val ? L"true" : L"false");
            }
        }

        /**
         * Create a column from a string convertible value
         *
         * @tparam U the type of the value
         * @param val the value
         */
        template<class U>
        csvrowcolumn(const U &val) {
            if constexpr (util::is_u8_string_v<T>) {
                value = std::to_string(val);
            } else if constexpr (util::is_u16_string_v<T>) {
                value = std::to_wstring(val);
            }
        }

        csvrowcolumn &operator=(const csvrowcolumn<T> &) = default;

        /**
         * Assign nothing to this column
         *
         * @return this
         */
        csvrowcolumn &operator=(std::nullptr_t) {
            if constexpr (util::is_u8_string_v<T>) {
                value = "";
            } else if constexpr (util::is_u8_string_v<T>) {
                value = L"";
            }

            return *this;
        }

        csvrowcolumn &operator=(const T &val) {
            value = T();
            value += '"';
            value += val;
            value += '"';
            return *this;
        }

        csvrowcolumn &operator=(const char *val) {
            if constexpr (util::is_u8_string_v<T>) {
                value = '"' + T(val) + '"';
            } else if constexpr (util::is_u16_string_v<T>) {
                value = L'"' + util::string_to_wstring(val) + L'"';
            }
            return *this;
        }

        CSV_REQUIRES(T, std::wstring)
        csvrowcolumn &operator=(const wchar_t *val) {
            value = L'"' + T(val) + L'"';
            return *this;
        }

        csvrowcolumn &operator=(bool val) {
            if constexpr (util::is_u8_string_v<T>) {
                value = std::string(val ? "true" : "false");
            } else if constexpr (util::is_u16_string_v<T>) {
                value = std::wstring(val ? L"true" : L"false");
            }

            return *this;
        }

        csvrowcolumn &operator=(char val) {
            value = T();
            value += '"';
            value += val;
            value += '"';
            return *this;
        }

        CSV_REQUIRES(T, std::wstring)
        csvrowcolumn &operator=(wchar_t val) {
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

        csvrowcolumn<T> operator++(int) {
            if (isDecimal()) {
                this->operator=(this->as<long long>() + 1);
            } else {
                this->operator=(this->as<long double>() + 1.0);
            }
            return *this;
        }

        csvrowcolumn<T> operator--(int) {
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
    class const_csvrow {
    public:
        CSV_CHECK_T_SUPPORTED

        const_csvrow() : columns() {}

        explicit const_csvrow(std::nullptr_t) : columns() {}

        const_csvrow(const std::vector<csvrowcolumn<T>> &data) : columns(data) {}

        template<class U>
        const_csvrow(const std::vector<U> &data) : columns(data.size()) {
            for (size_t i = 0; i < data.size(); i++) {
                columns[i] = csvrowcolumn<T>(data[i]);
            }
        }

        const_csvrow(const std::initializer_list<csvrowcolumn<T>> &data) : columns(data) {}

        template<class U>
        const_csvrow(const std::initializer_list<U> &list) : columns() {
            columns.reserve(list.size());
            for (const U &val: list) {
                columns.emplace_back(val);
            }
        }

        const csvrowcolumn<T> &at(size_t index) const {
            return this->columns.at(index);
        }

        const csvrowcolumn<T> &operator[](size_t columnIndex) const {
            return this->at(columnIndex);
        }

        CSV_NODISCARD bool operator==(const const_csvrow<T> &other) const {
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

        CSV_NODISCARD bool operator!=(const const_csvrow<T> &other) const {
            return !this->operator==(other);
        }

        CSV_NODISCARD virtual typename std::vector<csvrowcolumn<T>>::const_iterator begin() const noexcept {
            return columns.begin();
        }

        CSV_NODISCARD virtual typename std::vector<csvrowcolumn<T>>::const_iterator end() const noexcept {
            return columns.end();
        }

        CSV_NODISCARD size_t size() const noexcept {
            return columns.size();
        }

        CSV_NODISCARD bool empty() const noexcept {
            return columns.empty();
        }

        CSV_NODISCARD T to_string(char separator = ';') const {
            if constexpr (util::is_u8_string_v<T>) {
                return to_string_impl<std::stringstream>(separator);
            } else if constexpr (util::is_u16_string_v<T>) {
                return to_string_impl<std::wstringstream>(separator);
            }
        }

    protected:
        const_csvrow &operator=(const const_csvrow<T> &) = default;

        template<class U>
        CSV_NODISCARD T to_string_impl(char separator) const {
            U ss;
            for (const csvrowcolumn<T> &col : columns) {
                ss << col.rawValue() << separator;
            }

            return ss.str();
        }

        std::vector<csvrowcolumn<T>> columns;
    };

    template<class T>
    class csvrow : public const_csvrow<T> {
    public:
        CSV_CHECK_T_SUPPORTED

        static csvrow<T> parse(const T &value, const char separator) {
            csvrow row(nullptr);
            if (!value.empty()) {
                for (const T &col : util::splitString(value, separator)) {
                    row << csvrowcolumn<T>::parse(col);
                }
            }

            return row;
        }

        using const_csvrow<T>::const_csvrow;

        csvrow &operator=(const csvrow<T> &other) {
            this->columns = other.columns;
            return *this;
        }

        csvrow &operator=(const const_csvrow<T> &other) {
            this->columns = other.columns;
            return *this;
        }

        csvrow &operator=(const std::vector<csvrowcolumn<T>> &data) {
            this->columns = data;
            return *this;
        }

        template<class U>
        csvrow &operator=(const std::vector<U> &data) {
            for (size_t i = 0; i < data.size(); i++) {
                this->at(i).operator=(data.at(i));
            }

            return *this;
        }

        csvrow &operator=(const std::initializer_list<csvrowcolumn<T>> &data) {
            this->columns = std::vector<csvrowcolumn<T>>(data);
            return *this;
        }

        template<class U>
        csvrow &operator=(const std::initializer_list<U> &data) {
            for (size_t i = 0; i < data.size(); i++) {
                this->at(i).operator=(data[i]);
            }

            return *this;
        }

        csvrow operator+(const const_csvrow<T> &other) const {
            csvrow<T> res(this->columns);
            res << other;

            return res;
        }

        csvrow &operator+=(const const_csvrow<T> &other) {
            for (const csvrowcolumn<T> &col : other) {
                this->columns.push_back(col);
            }

            return *this;
        }

        csvrowcolumn<T> &at(size_t index) {
            if (index >= this->columns.size()) {
                for (size_t i = this->columns.size(); i <= index; i++) {
                    this->columns.emplace_back(nullptr);
                }
            }

            return this->columns.at(index);
        }

        csvrowcolumn<T> &operator[](size_t columnIndex) {
            return this->at(columnIndex);
        }

        csvrow &operator<<(const csvrowcolumn<T> &col) {
            this->columns.push_back(col);
            return *this;
        }

        csvrow &operator<<(const const_csvrow<T> &other) {
            for (const csvrowcolumn<T> &col : other) {
                *this << col;
            }

            return *this;
        }

        template<class U>
        csvrow &push(const U &val) {
            return this->operator<<(val);
        }

        csvrowcolumn<T> &get_next() {
            return this->columns.emplace_back(nullptr);
        }

        CSV_NODISCARD typename std::vector<csvrowcolumn<T>>::const_iterator begin() const noexcept {
            return this->columns.begin();
        }

        typename std::vector<csvrowcolumn<T>>::iterator begin() noexcept {
            return this->columns.begin();
        }

        CSV_NODISCARD typename std::vector<csvrowcolumn<T>>::const_iterator end() const noexcept {
            return this->columns.end();
        }

        typename std::vector<csvrowcolumn<T>>::iterator end() noexcept {
            return this->columns.end();
        }

        void clear() noexcept {
            this->columns.clear();
        }
    };

    template<class T>
    class basic_csv_file;

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

        template<class U, CSV_ENABLE_IF(util::is_any_of_v<U, char, wchar_t>) >
        static basic_csv_file<U> &endl(basic_csv_file<U> &f) {
            f.endline();
            return f;
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
                for (const csvrowcolumn<T> &col : this->rows[i]) {
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
            if (!value.empty()) {
                if (value[value.size() - 1] == '\n') {
                    rows.emplace_back(nullptr);
                }
            }
        }

        std::vector<csvrow<T>> rows;
        char separator;
    };

    template<class T>
    class basic_csv_file {
    private:
        using string_t = std::basic_string<T, std::char_traits<T>, std::allocator<T>>;
        using stream_t = std::basic_fstream<T, std::char_traits<T>>;
        using cache_iterator = typename std::map<int64_t, string_t>::const_iterator;
    public:
        static_assert(util::is_any_of_v<T, char, wchar_t>, "T must be one of [char, wchar_t]");

        class const_csv_file_row : public const_csvrow<string_t> {
        public:
            explicit const_csv_file_row(const csvrow<string_t> &data) : const_csvrow<string_t>(data) {}
        };

        class csv_file_row : public csvrow<string_t> {
        public:
            csv_file_row(basic_csv_file &file, const csvrow<string_t> &data, int64_t line) : csvrow<string_t>(data),
                                                                                             line(line), file(file) {}

            ~csv_file_row() {
                file.writeToFile(this->to_string(file.separator), line);
            }

        private:
            int64_t line;
            basic_csv_file &file;
        };

        explicit basic_csv_file(const string_t &path, size_t maxCached = 100, char separator = ';') : separator(
                separator), path(path), cache(), maxCached(maxCached) {
            currentLine = getMaxLineIndex();
        }

        basic_csv_file &operator=(const basic_csv<string_t> &csv) {
            clear();
            return this->push(csv);
        }

        basic_csv_file &operator<<(const char *val) {
            csvrow<string_t> row = getCurrentLine();

            row << csvrowcolumn<string_t>(val);
            writeToFile(row.to_string(separator), currentLine);

            return *this;
        }

        CSV_REQUIRES(T, std::wstring)
        basic_csv_file &operator<<(const wchar_t *val) {
            csvrow<string_t> row = getCurrentLine();

            row << csvrowcolumn<string_t>(val);
            writeToFile(row.to_string(separator), currentLine);

            return *this;
        }

        template<class U>
        basic_csv_file &operator<<(const U &val) {
            csvrow<string_t> row = getCurrentLine();

            row << csvrowcolumn<string_t>(val);
            writeToFile(row.to_string(separator), currentLine);

            return *this;
        }

        basic_csv_file &operator<<(const basic_csv<string_t> &el) {
            csvrow<string_t> line = getCurrentLine();

            basic_csv<string_t> csv = el;
            csv[0] = line + el[0];
            cache.insert_or_assign(currentLine, csv.to_string());

            writeCacheToFile();
            currentLine = getMaxLineIndex();

            return *this;
        }

        basic_csv_file<T> &operator<<(basic_csv_file<T> &(*val)(basic_csv_file<T> &)) {
            return val(*this);
        }

        friend basic_csv<string_t> &operator>>(basic_csv_file<T> &file, basic_csv<string_t> &csv) {
            file.writeCacheToFile();

            stream_t in = file.getStream(std::ios::in);
            in >> csv;
            in.close();

            return csv;
        }

        template<class U>
        basic_csv_file &push(const U &val) {
            return this->operator<<(val);
        }

        const_csv_file_row at(size_t line) const {
            if (line > getMaxLineIndex()) {
                throw std::runtime_error("The requested line index does not exist");
            }

            return const_csv_file_row(getLine(line));
        }

        csv_file_row at(size_t line) {
            return csv_file_row(*this, getOrCreateLine(line), line);
        }

        const_csv_file_row operator[](size_t line) const {
            return this->at(line);
        }

        csv_file_row operator[](size_t line) {
            return this->at(line);
        }

        basic_csv<string_t> to_basic_csv() {
            basic_csv<string_t> csv;
            *this >> csv;

            return csv;
        }

        basic_csv_file<T> &endline() {
            currentLine++;
            return *this;
        }

        CSV_NODISCARD int64_t size() const {
            return getMaxLineIndex() + 1;
        }

        void clear() {
            cache.clear();
            std::remove(util::string_as<std::string>(path).c_str());
        }

        void flush() {
            // If the cache isn't empty, write it to the file
            if (!cache.empty()) {
                writeCacheToFile();
            }
        }

        ~basic_csv_file() {
            flush();
        }

        static basic_csv_file<T> &endl(basic_csv_file<T> &c) {
            c.endline();
            return c;
        }

    private:
        void writeToFile(const string_t &row, int64_t line) {
            cache.insert_or_assign(line, row);

            if (cache.size() >= maxCached) {
                writeCacheToFile();
            }
        }

        csvrow<string_t> getOrCreateLine(int64_t line) {
            if (line <= getMaxLineIndex()) {
                return getLine(line);
            } else {
                if (line > currentLine) currentLine = line;
                cache.insert_or_assign(line, string_t());

                return csvrow<string_t>(nullptr);
            }
        }

        CSV_NODISCARD csvrow<string_t> getLine(int64_t line) const {
            if (line > getMaxLineIndex()) {
                return csvrow<string_t>(nullptr);
            }

            const cache_iterator it = cache.find(line);
            if (it == cache.end()) {
                stream_t in = getStream(std::ios::in);
                gotoLine(in, line);

                string_t value;
                std::getline(in, value);
                in.close();

                if (value.empty()) {
                    return csvrow<string_t>(nullptr);
                } else {
                    return csvrow<string_t>::parse(value, separator);
                }
            } else {
                return csvrow<string_t>::parse(it->second, separator);
            }
        }

        CSV_NODISCARD csvrow<string_t> getCurrentLine() const {
            return getLine(currentLine);
        }

        CSV_NODISCARD stream_t getStream(std::ios_base::openmode openMode) const {
            return stream_t(path, openMode);
        }

        CSV_NODISCARD int64_t getMaxLineIndex() const {
            stream_t in = getStream(std::ios::in);
            int64_t fLines = std::count(std::istreambuf_iterator<T>(in), std::istreambuf_iterator<T>(), '\n');
            in.close();

            if (cache.empty()) {
                return fLines;
            } else {
                return std::max(fLines, cache.rbegin()->first);
            }
        }

        template<class U = string_t>
        CSV_NODISCARD U getTmpFile() const {
            string_t out = path;
            if constexpr (util::is_u8_string_v<string_t>) {
                out += ".tmp";
            } else if constexpr (util::is_u16_string_v<string_t>) {
                out += L".tmp";
            }

            return util::string_as<U>(out);
        }

        void writeCacheToFile() {
            writeDataToFile(cache);
            cache.clear();
        }

        void writeDataToFile(std::map<int64_t, string_t> &values) {
            std::remove(getTmpFile<std::string>().c_str());
            stream_t out(getTmpFile(), std::ios::out | std::ios::app);
            stream_t in = getStream(std::ios::in);

            int64_t i = 0;
            string_t current;
            in.seekg(std::ios::beg);
            while (std::getline(in, current)) {
                if (i > 0) {
                    out << std::endl;
                }

                const cache_iterator it = values.find(i++);
                if (it == values.end()) {
                    out << current;
                } else {
                    out << it->second;
                    values.erase(it);
                }
            }

            in.close();

            // Write all values that were not already written to the file into the file
            if (!values.empty()) {
                const int64_t max = values.rbegin()->first;
                for (; i <= max; i++) {
                    if (i > 0) {
                        out << std::endl;
                    }

                    const cache_iterator it = values.find(i);
                    if (it != values.end()) {
                        out << it->second;
                    }
                }
            }

            // Append a new line at the end, if required
            if (i <= currentLine) {
                out << std::endl;
            }

            out.flush();
            out.close();

            std::remove(util::string_as<std::string>(path).c_str());
            std::rename(getTmpFile<std::string>().c_str(), util::string_as<std::string>(path).c_str());
        }

        static void gotoLine(stream_t &stream, int64_t num) {
            stream.seekg(std::ios::beg);
            for (size_t i = 0; i < num; i++) {
                stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }

        size_t maxCached;
        std::map<int64_t, string_t> cache;
        char separator;
        string_t path;
        int64_t currentLine;
    };

    using csv = basic_csv<std::string>;
    using w_csv = basic_csv<std::wstring>;

    using csv_file = basic_csv_file<char>;
}

#undef CSV_NODISCARD
#undef CSV_CHECK_T_SUPPORTED
#undef CSV_WINDOWS
#undef CSV_UNIX
#undef CSV_REQUIRES
#undef CSV_ENABLE_IF

#endif //MARKUSJX_CSV_HPP
