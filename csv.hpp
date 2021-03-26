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

        /**
         * Assign a string value to this column
         *
         * @param val the value to assign
         * @return this
         */
        csvrowcolumn &operator=(const T &val) {
            value = T();
            value += '"';
            value += val;
            value += '"';
            return *this;
        }

        /**
         * Assign a char array to this
         *
         * @param val the array
         * @return this
         */
        csvrowcolumn &operator=(const char *val) {
            if constexpr (util::is_u8_string_v<T>) {
                value = '"' + T(val) + '"';
            } else if constexpr (util::is_u16_string_v<T>) {
                value = L'"' + util::string_to_wstring(val) + L'"';
            }
            return *this;
        }

        /**
         * Assign a wide char array to this. Requires T to be a wide string.
         *
         * @param val the value to assign
         * @return this
         */
        CSV_REQUIRES(T, std::wstring)
        csvrowcolumn &operator=(const wchar_t *val) {
            value = L'"' + T(val) + L'"';
            return *this;
        }

        /**
         * Assign a bool to this column
         *
         * @param val the value to assign
         * @return this
         */
        csvrowcolumn &operator=(bool val) {
            if constexpr (util::is_u8_string_v<T>) {
                value = std::string(val ? "true" : "false");
            } else if constexpr (util::is_u16_string_v<T>) {
                value = std::wstring(val ? L"true" : L"false");
            }

            return *this;
        }

        /**
         * Assign a character to this column
         *
         * @param val the character to assign
         * @return this
         */
        csvrowcolumn &operator=(char val) {
            value = T();
            value += '"';
            value += val;
            value += '"';
            return *this;
        }

        /**
         * Assign a wide char to this column. Only available if T = std::wstring
         *
         * @param val the wide char to assign
         * @return this
         */
        CSV_REQUIRES(T, std::wstring)
        csvrowcolumn &operator=(wchar_t val) {
            value = T();
            value += L'"';
            value += val;
            value += L'"';
            return *this;
        }

        /**
         * Assign a std::string to this column. Only available if T = std::wstring
         *
         * @param val the wide string to assign
         * @return this
         */
        CSV_REQUIRES(T, std::wstring)
        csvrowcolumn &operator=(const std::string &val) {
            value = L'"' + util::string_to_wstring(val) + L'"';
            return *this;
        }

        /**
         * Assign any value to this column
         *
         * @tparam U the type of the value
         * @param val the value to assign
         * @return this
         */
        template<class U>
        csvrowcolumn &operator=(const U &val) {
            if constexpr (util::is_u8_string_v<T>) {
                value = std::to_string(val);
            } else if constexpr (util::is_u16_string_v<T>) {
                value = std::to_wstring(val);
            }

            return *this;
        }

        /**
         * Get the raw string value of this column
         *
         * @return the raw value
         */
        CSV_NODISCARD T rawValue() const {
            return value;
        }

        /**
         * Get this as a string
         *
         * @return the string
         */
        CSV_NODISCARD operator T() const {
            if (value[0] == '"') {
                return value.substr(1, value.size() - 2);
            } else {
                throw std::runtime_error("The stored value is not a string");
            }
        }

        /**
         * Get this as a char.
         * Only available if T = std::string
         *
         * @return the char
         */
        CSV_REQUIRES(T, std::string)
        CSV_NODISCARD operator char() const {
            T val = this->operator T();
            if (val.size() == 1) {
                return val[0];
            } else {
                throw std::runtime_error("The value is not a character");
            }
        }

        /**
         * Get this as a wide char.
         * Only available if T = std::wstring
         *
         * @return the wide char
         */
        CSV_REQUIRES(T, std::wstring)
        CSV_NODISCARD operator wchar_t() const {
            T val = this->operator T();
            if (val.size() == 1) {
                return val[0];
            } else {
                throw std::runtime_error("The value is not a character");
            }
        }

        /**
         * Get this as an integer
         *
         * @return the integer
         */
        CSV_NODISCARD operator int() const {
            return util::stringToNumber<int>(value, std::stoi);
        }

        /**
         * Get this as a long
         *
         * @return this as a long value
         */
        CSV_NODISCARD operator long() const {
            return util::stringToNumber<long>(value, std::stol);
        }

        /**
         * Get this as a unsigned long
         *
         * @return this as a unsigned long
         */
        CSV_NODISCARD operator unsigned long() const {
            return util::stringToNumber<unsigned long>(value, std::stoul);
        }

        /**
         * Get this as a long long
         *
         * @return this as a long long
         */
        CSV_NODISCARD operator long long() const {
            return util::stringToNumber<long long>(value, std::stoll);
        }

        /**
         * Get this as a unsigned long long
         *
         * @return this as a unsigned long long
         */
        CSV_NODISCARD operator unsigned long long() const {
            return util::stringToNumber<unsigned long long>(value, std::stoull);
        }

        /**
         * Get this as a double
         *
         * @return this as a double
         */
        CSV_NODISCARD operator double() const {
            return util::stringToNumberAlt<double>(value, std::stod);
        }

        /**
         * Get this as a long double
         *
         * @return this as a long double
         */
        CSV_NODISCARD operator long double() const {
            return util::stringToNumberAlt<long double>(value, std::stold);
        }

        /**
         * Get this as a float
         *
         * @return this as a float
         */
        CSV_NODISCARD operator float() const {
            return util::stringToNumberAlt<float>(value, std::stof);
        }

        /**
         * Get this as a bool
         *
         * @return this as a bool
         */
        CSV_NODISCARD operator bool() const {
            if constexpr (util::is_u8_string_v<T>) {
                return op_bool_impl(value, "true", "false");
            } else if constexpr (util::is_u16_string_v<T>) {
                return op_bool_impl(value, L"true", L"false");
            }
        }

        /**
         * Get this as a value specified by U
         *
         * @tparam U the type to convert to
         * @return this as the type
         */
        template<class U>
        CSV_NODISCARD U as() const {
            return this->operator U();
        }

        /**
         * Operator equals with a value
         *
         * @tparam U the type of the value
         * @param val the value
         * @return true, if the values match
         */
        template<class U>
        CSV_NODISCARD bool operator==(const U &val) const {
            return this->as<U>() == val;
        }

        /**
         * Operator unequals with a value
         *
         * @tparam U the type of the value
         * @param val the value
         * @return true if the values do not match
         */
        template<class U>
        CSV_NODISCARD bool operator!=(const U &val) const {
            return this->as<U>() != val;
        }

        /**
         * Operator equals with another column
         *
         * @param other the column to compare with
         * @return true if the columns match
         */
        CSV_NODISCARD bool operator==(const csvrowcolumn<T> &other) const {
            if (this == &other) {
                return true;
            } else {
                return this->rawValue() == other.rawValue();
            }
        }

        /**
         * Operator unequals with another column
         *
         * @param other the column to compare with
         * @return true if the columns do not match
         */
        CSV_NODISCARD bool operator!=(const csvrowcolumn<T> &other) const {
            if (this == &other) {
                return false;
            } else {
                return this->rawValue() != other.rawValue();
            }
        }

        /**
         * Operator smaller than with another column
         *
         * @param other the column to compare with
         * @return true if this is smaller than other
         */
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

        /**
         * Operator smaller than with a value
         *
         * @tparam U the type of the value
         * @param val the value to compare to
         * @return true if this's value is smaller than val
         */
        template<class U>
        CSV_NODISCARD bool operator<(const U &val) const {
            return this->as<U>() < val;
        }

        /**
         * Operator smaller equals with another column
         *
         * @param other the column to compare with
         * @return true if this is smaller than or equal to other
         */
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

        /**
         * Operator smaller equals with a value
         *
         * @tparam U the type of the value
         * @param val the value to compare to
         * @return true if this's value is smaller than val
         */
        template<class U>
        CSV_NODISCARD bool operator<=(const U &val) const {
            return this->as<U>() <= val;
        }

        /**
         * Operator greater with another column
         *
         * @param other the column to compare with
         * @return true if this is greater than other
         */
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

        /**
         * Operator greater with a value
         *
         * @tparam U the type of the value
         * @param val the value to compare to
         * @return true if this's value is greater than val
         */
        template<class U>
        CSV_NODISCARD bool operator>(const U &val) const {
            return this->as<U>() > val;
        }

        /**
         * Operator greater equal with another column
         *
         * @param other the column to compare with
         * @return true if this is greater than or equal to other
         */
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

        /**
         * Operator greater equal with a value
         *
         * @tparam U the type of the value
         * @param val the value to compare to
         * @return true if this's value is greater than or equal to val
         */
        template<class U>
        CSV_NODISCARD bool operator>=(const U &val) const {
            return this->as<U>() >= val;
        }

        /**
         * Prefix increment operator
         *
         * @return this after its value was increased
         */
        csvrowcolumn<T> &operator++() {
            if (isDecimal()) {
                this->operator=(this->as<long long>() + 1);
            } else {
                this->operator=(this->as<long double>() + 1.0);
            }
            return *this;
        }

        /**
         * Postfix increment operator
         *
         * @return the value of this before it was increased
         */
        csvrowcolumn<T> operator++(int) {
            csvrowcolumn<T> old = *this;
            this->operator++();

            return old;
        }

        /**
         * Prefix decrement operator
         *
         * @return this after its value was decreased
         */
        csvrowcolumn<T> &operator--() {
            if (isDecimal()) {
                this->operator=(this->as<long long>() - 1);
            } else {
                this->operator=(this->as<long double>() - 1.0);
            }
            return *this;
        }

        /**
         * Postfix decrement operator
         *
         * @return this before its value was decreased
         */
        csvrowcolumn<T> operator--(int) {
            csvrowcolumn<T> old = *this;
            this->operator--();
            return old;
        }

        /**
         * Operator add.
         * This's value must be a number or string in order for this to work.
         *
         * @param val the column to add
         * @return this with the column added
         */
        CSV_NODISCARD csvrowcolumn<T> operator+(const csvrowcolumn<T> &val) const {
            if (this->isFloatingPoint() || val.isFloatingPoint()) {
                return csvrowcolumn(this->as<long double>() + val.as<long double>());
            } else if (this->isNumber() && val.isNumber()) {
                return csvrowcolumn(this->as<long long>() + val.as<long long>());
            } else {
                return csvrowcolumn(this->as<T>() + val.as<T>());
            }
        }

        /**
         * Operator odd
         *
         * @tparam U the type of the value to add
         * @param val the value to add
         * @return this with the value added
         */
        template<class U>
        CSV_NODISCARD csvrowcolumn<T> operator+(const U &val) const {
            return csvrowcolumn(this->as<U>() + val);
        }

        /**
         * Operator add assign.
         *
         * @tparam U the type of the value to add
         * @param val the value to add
         * @return this
         */
        template<class U>
        csvrowcolumn<T> &operator+=(const U &val) {
            return this->operator=(this->operator+(val));
        }

        /**
         * Operator minus.
         * This's value must be a number in order for this to work.
         *
         * @param val the column to subtract
         * @return this with the column subtracted
         */
        CSV_NODISCARD csvrowcolumn<T> operator-(const csvrowcolumn<T> &val) const {
            if (this->isFloatingPoint() || val.isFloatingPoint()) {
                return csvrowcolumn(this->as<long double>() - val.as<long double>());
            } else if (this->isNumber() && val.isNumber()) {
                return csvrowcolumn(this->as<long long>() - val.as<long long>());
            } else {
                throw std::runtime_error("The value is not a number");
            }
        }

        /**
         * Operator minus.
         * This's value must be a number in order for this to work.
         *
         * @tparam U the type of the value to subtract
         * @param val the value to subtract
         * @return this with the value subtracted
         */
        template<class U>
        CSV_NODISCARD csvrowcolumn<T> operator-(const U &val) const {
            return csvrowcolumn(this->as<U>() - val);
        }

        /**
         * Operator minus equals.
         * This's value must be a number in order for this to work.
         *
         * @tparam U the type of the value to subtract
         * @param val the value to subtract
         * @return this
         */
        template<class U>
        csvrowcolumn<T> &operator-=(const U &val) {
            return this->operator=(this->operator-(val));
        }

        /**
         * Operator multiply.
         * This's value must be a number in order for this to work.
         *
         * @param val the column to multiply this with
         * @return this with val multiplied
         */
        CSV_NODISCARD csvrowcolumn<T> operator*(const csvrowcolumn<T> &val) const {
            if (this->isFloatingPoint() || val.isFloatingPoint()) {
                return csvrowcolumn(this->as<long double>() * val.as<long double>());
            } else if (this->isNumber() && val.isNumber()) {
                return csvrowcolumn(this->as<long long>() * val.as<long long>());
            } else {
                throw std::runtime_error("The value is not a number");
            }
        }

        /**
         * Operator multiply.
         * This's value must be a number in order for this to work.
         *
         * @tparam U the type of the value to multiply this with
         * @param val the value to multiply this with
         * @return this with val multiplied
         */
        template<class U>
        CSV_NODISCARD csvrowcolumn<T> operator*(const U &val) const {
            return csvrowcolumn(this->as<U>() * val);
        }

        /**
         * Operator multiply equals.
         * This's value must be a number in order for this to work.
         *
         * @tparam U the type of the value to multiply this with
         * @param val the value to multiply this with
         * @return this
         */
        template<class U>
        csvrowcolumn<T> &operator*=(const U &val) {
            return this->operator=(this->operator*(val));
        }

        /**
         * Operator divide.
         * This's value must be a number in order for this to work.
         *
         * @param val the column to divide this by
         * @return this divided by val
         */
        CSV_NODISCARD csvrowcolumn<T> operator/(const csvrowcolumn<T> &val) const {
            if (this->isFloatingPoint() || val.isFloatingPoint()) {
                return csvrowcolumn(this->as<long double>() / val.as<long double>());
            } else if (this->isNumber() && val.isNumber()) {
                return csvrowcolumn(this->as<long long>() / val.as<long long>());
            } else {
                throw std::runtime_error("The value is not a number");
            }
        }

        /**
         * Operator divide.
         * This's value must be a number in order for this to work.
         *
         * @tparam U the type of the value to divide this by
         * @param val the value to divide this by
         * @return this divided by val
         */
        template<class U>
        CSV_NODISCARD csvrowcolumn<T> operator/(const U &val) const {
            return csvrowcolumn(this->as<U>() / val);
        }

        /**
         * Operator divide assign.
         * This's value must be a number in order for this to work.
         *
         * @tparam U the type of the value to divide this by
         * @param val the value to divide this by
         * @return this divided by val
         */
        template<class U>
        csvrowcolumn<T> &operator/=(const U &val) {
            return this->operator=(this->operator/(val));
        }

        /**
         * Check if this column is empty
         *
         * @return true if this column is empty
         */
        CSV_NODISCARD bool empty() const {
            return value.empty();
        }

        /**
         * Check if this column is a number
         *
         * @return true if this is a number
         */
        CSV_NODISCARD bool isNumber() const {
            const static std::regex number_regex("^-?[0-9]+(\\.[0-9]+)?$");
            return std::regex_match(value, number_regex);
        }

        /**
         * Check if this column is a decimal number
         *
         * @return true if this is a decimal
         */
        CSV_NODISCARD bool isDecimal() const {
            const static std::regex decimal_regex("^-?[0-9]+$");
            return std::regex_match(value, decimal_regex);
        }

        /**
         * Check if this column is a floating point integer
         *
         * @return true if this is a floating point integer
         */
        CSV_NODISCARD bool isFloatingPoint() const {
            const static std::regex float_regex("^-?[0-9]+\\.[0-9]+$");
            return std::regex_match(value, float_regex);
        }

    private:
        /**
         * The operator bool implementation
         *
         * @tparam U the type of the true and false values
         * @param val the value to convert to a bool
         * @param true_val the value that equals to "true"
         * @param false_val the value that equals to "false"
         * @return true or false depending whether val equals true_val or false_val
         */
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

        // The string value stored in this column
        T value;
    };

    /**
     * A constant csv row
     *
     * @tparam T the type of the row
     */
    template<class T>
    class const_csvrow {
    public:
        CSV_CHECK_T_SUPPORTED

        /**
         * Create an empty row
         */
        const_csvrow() : columns() {}

        /**
         * Copy constructor
         *
         * @param other the object to copy
         */
        const_csvrow(const const_csvrow &other) : columns(other.columns) {}

        /**
         * Create an empty row
         */
        explicit const_csvrow(std::nullptr_t) : columns() {}

        /**
         * Create a row from a data vector
         *
         * @param data the data vector
         */
        const_csvrow(const std::vector<csvrowcolumn<T>> &data) : columns(data) {}

        /**
         * Create a row from a data vector
         *
         * @tparam U the type of the data vector
         * @param data the data vector
         */
        template<class U>
        const_csvrow(const std::vector<U> &data) : columns(data.size()) {
            for (size_t i = 0; i < data.size(); i++) {
                columns[i] = csvrowcolumn<T>(data[i]);
            }
        }

        /**
         * Create a row from a std::initializer_list
         *
         * @param data the std::initializer_list
         */
        const_csvrow(const std::initializer_list<csvrowcolumn<T>> &data) : columns(data) {}

        /**
         * Create a row from a std::initializer_list
         *
         * @tparam U the type of the list
         * @param list the data list
         */
        template<class U>
        const_csvrow(const std::initializer_list<U> &list) : columns() {
            columns.reserve(list.size());
            for (const U &val: list) {
                columns.emplace_back(val);
            }
        }

        /**
         * This is const, so just don't.
         */
        const_csvrow &operator=(const const_csvrow &) = delete;

        /**
         * This is const, so just don't.
         */
        const_csvrow &operator=(const_csvrow &&) = delete;

        /**
         * Get a csv column at an index
         *
         * @param index the index of the column to get
         * @return the retrieved column
         */
        const csvrowcolumn<T> &at(size_t index) const {
            return this->columns.at(index);
        }

        /**
         * Get a csv column at an index
         *
         * @param index the index of the column to get
         * @return the retrieved column
         */
        const csvrowcolumn<T> &operator[](size_t columnIndex) const {
            return this->at(columnIndex);
        }

        /**
         * Operator equals
         *
         * @param other the row to compare this to
         * @return true if this equals other
         */
        CSV_NODISCARD bool operator==(const const_csvrow<T> &other) const {
            if (this == &other) {
                // Early return if this matches other exactly
                return true;
            }

            if (this->size() == other.size()) {
                // If the sizes match, compare the individual values of the columns
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

        /**
         * Operator unequals
         *
         * @param other the row to compare this with
         * @return true if this is not equal to other
         */
        CSV_NODISCARD bool operator!=(const const_csvrow<T> &other) const {
            return !this->operator==(other);
        }

        /**
         * Get the begin iterator
         *
         * @return the begin iterator
         */
        CSV_NODISCARD virtual typename std::vector<csvrowcolumn<T>>::const_iterator begin() const noexcept {
            return columns.begin();
        }

        /**
         * Get the end iterator
         *
         * @return the end iterator
         */
        CSV_NODISCARD virtual typename std::vector<csvrowcolumn<T>>::const_iterator end() const noexcept {
            return columns.end();
        }

        /**
         * Get the number of columns in this row
         *
         * @return the number of columns in this row
         */
        CSV_NODISCARD size_t size() const noexcept {
            return columns.size();
        }

        /**
         * Check if this row is empty
         *
         * @return true if this row is empty
         */
        CSV_NODISCARD bool empty() const noexcept {
            return columns.empty();
        }

        /**
         * Convert this row to a string
         *
         * @param separator the separator to use
         * @return this as a string
         */
        CSV_NODISCARD T to_string(char separator = ';') const {
            if constexpr (util::is_u8_string_v<T>) {
                return to_string_impl<std::stringstream>(separator);
            } else if constexpr (util::is_u16_string_v<T>) {
                return to_string_impl<std::wstringstream>(separator);
            }
        }

    protected:
        /**
         * The to string implementation
         *
         * @tparam U the type of the string stream to use
         * @param separator the separator to use
         * @return this as a string
         */
        template<class U>
        CSV_NODISCARD T to_string_impl(char separator) const {
            U ss;

            // Write all values with the separator at the end to the stream
            for (const csvrowcolumn<T> &col : columns) {
                ss << col.rawValue() << separator;
            }

            return ss.str();
        }

        // The columns in this row
        std::vector<csvrowcolumn<T>> columns;
    };

    /**
     * A csv row
     *
     * @tparam T the type of the row
     */
    template<class T>
    class csvrow : public const_csvrow<T> {
    public:
        CSV_CHECK_T_SUPPORTED

        /**
         * Parse a row string
         *
         * @param value the string value to parse
         * @param separator the separator used in the string
         * @return the parsed row
         */
        static csvrow<T> parse(const T &value, const char separator) {
            csvrow row(nullptr);
            if (!value.empty()) {
                // Write all columns in the row's column vector
                for (const T &col : util::splitString(value, separator)) {
                    row << csvrowcolumn<T>::parse(col);
                }
            }

            return row;
        }

        using const_csvrow<T>::const_csvrow;

        /**
         * Copy constructor
         *
         * @param row the row to copy
         */
        csvrow(const csvrow<T> &row) : const_csvrow<T>(row) {}

        /**
         * Assign operator
         *
         * @param other the row to assign to this
         * @return this
         */
        csvrow &operator=(const csvrow<T> &other) {
            this->columns = other.columns;
            return *this;
        }

        /**
         * Assign operator
         *
         * @param other the row to assign to this
         * @return this
         */
        csvrow &operator=(const const_csvrow<T> &other) {
            this->columns = other.columns;
            return *this;
        }

        /**
         * Assign a data vector to this
         *
         * @param data the vector to assign to this
         * @return this
         */
        csvrow &operator=(const std::vector<csvrowcolumn<T>> &data) {
            this->columns = data;
            return *this;
        }

        /**
         * Assign a data vector to this
         *
         * @tparam U the type of the data vector
         * @param data the vector to assign to this
         * @return this
         */
        template<class U>
        csvrow &operator=(const std::vector<U> &data) {
            for (size_t i = 0; i < data.size(); i++) {
                this->at(i).operator=(data.at(i));
            }

            return *this;
        }

        /**
         * Assign a std::initializer_list vector to this
         *
         * @param data the list to assign to this
         * @return this
         */
        csvrow &operator=(const std::initializer_list<csvrowcolumn<T>> &data) {
            this->columns = std::vector<csvrowcolumn<T>>(data);
            return *this;
        }

        /**
         * Assign a std::initializer_list vector to this
         *
         * @tparam U  the type of the initializer_list
         * @param data the list to assign to this
         * @return this
         */
        template<class U>
        csvrow &operator=(const std::initializer_list<U> &data) {
            for (size_t i = 0; i < data.size(); i++) {
                this->at(i).operator=(data[i]);
            }

            return *this;
        }

        /**
         * Operator add
         *
         * @param other the row to add to this
         * @return this with the values of other added
         */
        csvrow operator+(const const_csvrow<T> &other) const {
            csvrow<T> res(this->columns);
            res << other;

            return res;
        }

        /**
         * Operator assign add
         *
         * @param other the row to add to this
         * @return this
         */
        csvrow &operator+=(const const_csvrow<T> &other) {
            for (const csvrowcolumn<T> &col : other) {
                this->columns.push_back(col);
            }

            return *this;
        }

        /**
         * Get the column at an index
         *
         * @param index the index of the column to get
         * @return the column at index
         */
        csvrowcolumn<T> &at(size_t index) {
            if (index >= this->columns.size()) {
                for (size_t i = this->columns.size(); i <= index; i++) {
                    this->columns.emplace_back(nullptr);
                }
            }

            return this->columns.at(index);
        }

        /**
         * Get the column at an index
         *
         * @param index the index of the column to get
         * @return the column at index
         */
        csvrowcolumn<T> &operator[](size_t columnIndex) {
            return this->at(columnIndex);
        }

        /**
         * Operator <<
         *
         * @param col the column to add to this
         * @return this
         */
        csvrow &operator<<(const csvrowcolumn<T> &col) {
            this->columns.push_back(col);
            return *this;
        }

        /**
         * Operator <<
         *
         * @param col the row to add to this
         * @return this
         */
        csvrow &operator<<(const const_csvrow<T> &other) {
            for (const csvrowcolumn<T> &col : other) {
                *this << col;
            }

            return *this;
        }

        /**
         * Add a value to this
         *
         * @tparam U the type of the value to add
         * @param val the value to add
         * @return this
         */
        template<class U>
        csvrow &push(const U &val) {
            return this->operator<<(val);
        }

        /**
         * Get the next column.
         * Adds an empty column to this and returns it
         *
         * @return the next column
         */
        csvrowcolumn<T> &get_next() {
            return this->columns.emplace_back(nullptr);
        }

        CSV_NODISCARD typename std::vector<csvrowcolumn<T>>::const_iterator begin() const noexcept {
            return this->columns.begin();
        }

        /**
         * Get the begin iterator
         *
         * @return the begin iterator
         */
        typename std::vector<csvrowcolumn<T>>::iterator begin() noexcept {
            return this->columns.begin();
        }

        CSV_NODISCARD typename std::vector<csvrowcolumn<T>>::const_iterator end() const noexcept {
            return this->columns.end();
        }

        /**
         * Get the end iterator
         *
         * @return the end iterator
         */
        typename std::vector<csvrowcolumn<T>>::iterator end() noexcept {
            return this->columns.end();
        }

        /**
         * Clear this row
         */
        void clear() noexcept {
            this->columns.clear();
        }

        /**
         * Remove a value from this row
         *
         * @param index the index of the value to remove
         */
        void remove(size_t index) {
            this->columns.erase(this->columns.begin() + index);
        }
    };

    /**
     * A csv file
     *
     * @tparam T the char type of the file
     */
    template<class T>
    class basic_csv_file;

    /**
     * A csv object
     *
     * @tparam T the string type of the object. Must be either std::string or std::wstring
     */
    template<class T>
    class basic_csv {
    public:
        CSV_CHECK_T_SUPPORTED

        /**
         * Parse a csv string
         *
         * @param value the string to parse
         * @param separator the separator to use
         * @return the parsed csv object
         */
        static basic_csv<T> parse(const T &value, const char separator = ';') {
            basic_csv<T> csv(separator);
            csv.parseImpl(value);

            return csv;
        }

        /**
         * End the line
         *
         * @param c the csv object to add a new line
         * @return the csv object
         */
        static basic_csv<T> &endl(basic_csv<T> &c) {
            c.rows.emplace_back(nullptr);
            return c;
        }

        /**
         * End the line of a csv file
         *
         * @tparam U the type of the file
         * @param f the file to add a new line to
         * @return the file
         */
        template<class U, CSV_ENABLE_IF(util::is_any_of_v<U, char, wchar_t>) >
        static basic_csv_file<U> &endl(basic_csv_file<U> &f) {
            f.endline();
            return f;
        }

        /**
         * Create an csv object
         *
         * @param separator the separator to use
         */
        explicit basic_csv(char separator = ';') : rows(), separator(separator) {}

        /**
         * Create an empty csv object
         *
         * @param separator the separator to use
         */
        basic_csv(std::nullptr_t, char separator = ';') : rows(), separator(separator) {}

        /**
         * Create an csv object from a string
         *
         * @param value the string to parse
         * @param separator the separator to use
         */
        basic_csv(const T &value, char separator = ';') : rows(), separator(separator) {
            this->parseImpl(value);
        }

        /**
         * Create an csv object from a string.
         * Only available if T = std::wstring
         *
         * @param value the csv string to parse
         * @param separator the separator to use
         */
        CSV_REQUIRES(T, std::wstring)
        basic_csv(const std::string &value, char separator = ';') : rows(), separator(separator) {
            this->parseImpl(util::string_to_wstring(value));
        }

        /**
         * Create an csv object from a char array
         *
         * @param value the string char array to parse
         * @param separator the separator to use
         */
        basic_csv(const char *value, char separator = ';') : rows(), separator(separator) {
            if constexpr (util::is_u8_string_v<T>) {
                this->parseImpl(T(value));
            } else if constexpr (util::is_u16_string_v<T>) {
                this->parseImpl(util::string_to_wstring(value));
            }
        }

        /**
         * Create an csv object from a wide char array
         * Only available if T = std::wstring
         *
         * @param value the wide char array to parse
         * @param separator the separator to use
         */
        CSV_REQUIRES(T, std::wstring)
        basic_csv(const wchar_t *value, char separator = ';') : rows(), separator(separator) {
            this->parseImpl(T(value));
        }

        /**
         * Create an csv object from a data vector
         *
         * @param data the data vector
         * @param separator the separator to use
         */
        basic_csv(const std::vector<csvrow<T>> &data, char separator = ';') : rows(data), separator(separator) {}

        /**
         * Create an csv object from a data vector
         *
         * @param data the data vector
         * @param separator the separator to use
         */
        basic_csv(const std::vector<csvrowcolumn<T>> &data, char separator = ';') : rows({csvrow<T>(data)}),
                                                                                    separator(separator) {}

        /**
         * Create an csv object from a std::initializer_list
         *
         * @param data the data initializer_list
         * @param separator the separator to use
         */
        basic_csv(const std::initializer_list<csvrow<T>> &data, char separator = ';') : rows(data),
                                                                                        separator(separator) {}

        /**
         * Create an csv object from a std::initializer_list
         *
         * @param data the data initializer_list
         * @param separator the separator to use
         */
        basic_csv(const std::initializer_list<csvrowcolumn<T>> &data, char separator = ';') : rows({csvrow<T>(data)}),
                                                                                              separator(separator) {}

        /**
         * Get a row at an index.
         * Inserts the row if it doesn't exist.
         *
         * @param index the index of the row to get
         * @return the retrieved row
         */
        csvrow<T> &at(size_t index) {
            if (index >= this->rows.size()) {
                for (size_t i = rows.size(); i <= index; i++) {
                    rows.emplace_back(nullptr);
                }
            }

            return rows[index];
        }

        /**
         * Get a row at an index
         *
         * @param index the index of the row to get
         * @return the retrieved row
         */
        const csvrow<T> &at(size_t index) const {
            return rows[index];
        }

        /**
         * Get a row at an index.
         * Inserts the row if it doesn't exist.
         *
         * @param index the index of the row to get
         * @return the retrieved row
         */
        csvrow<T> &operator[](size_t index) {
            return this->at(index);
        }

        /**
         * Get a row at an index
         *
         * @param index the index of the row to get
         * @return the retrieved row
         */
        const csvrow<T> &operator[](size_t index) const {
            return this->at(index);
        }

        /**
         * Append a csv object to this
         *
         * @param other the object to add
         * @return this
         */
        basic_csv<T> &operator<<(const basic_csv<T> &other) {
            for (const csvrow<T> &row : other) {
                rows.push_back(row);
            }

            return *this;
        }

        /**
         * Append a column to this
         *
         * @param col the column to append
         * @return this
         */
        basic_csv<T> &operator<<(const csvrowcolumn<T> &col) {
            get_current() << col;
            return *this;
        }

        /**
         * Append a row to this
         *
         * @param row the row to append
         * @return this
         */
        basic_csv<T> &operator<<(const csvrow<T> &row) {
            this->rows.push_back(row);
            return *this;
        }

        /**
         * Append a value to this
         *
         * @tparam U the type of the value
         * @param val the value to add
         * @return this
         */
        template<class U>
        basic_csv<T> &operator<<(const U &val) {
            get_current().get_next() = val;
            return *this;
        }

        /**
         * Operator equals
         *
         * @param other the csv object to compare this with
         * @return true if the objects match
         */
        CSV_NODISCARD bool operator==(const basic_csv<T> &other) const {
            if (this == &other) {
                return true;
            }

            if (this->size() == other.size()) {
                // If the sizes match, check if all other values match
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

        /**
         * Operator unequals
         *
         * @param other the csv object to compare this with
         * @return true if the objects do not match
         */
        CSV_NODISCARD bool operator!=(const basic_csv<T> &other) const {
            return !this->operator==(other);
        }

        /**
         * Push a data vector to this
         *
         * @param data the data vector to append
         * @return this
         */
        basic_csv<T> &push(const std::vector<csvrowcolumn<T>> &data) {
            for (const csvrowcolumn<T> &col : data) {
                this->template operator<<(col);
            }
            return *this;
        }

        /**
         * Push a std::initializer_list to this
         *
         * @param data the data vector to append
         * @return this
         */
        basic_csv<T> &push(const std::initializer_list<csvrowcolumn<T>> &data) {
            for (const csvrowcolumn<T> &col : data) {
                this->template operator<<(col);
            }
            return *this;
        }

        /**
         * Push a data vector to this
         *
         * @param data the data vector to append
         * @return this
         */
        basic_csv<T> &push(const std::vector<csvrow<T>> &data) {
            for (const csvrow<T> &col : data) {
                this->push(col);
            }
            return *this;
        }

        /**
         * Push a std::initializer_list to this
         *
         * @param data the data vector to append
         * @return this
         */
        basic_csv<T> &push(const std::initializer_list<csvrow<T>> &data) {
            for (const csvrow<T> &col : data) {
                this->push(col);
            }
            return *this;
        }

        /**
         * Push a value to this
         *
         * @tparam U the type of the value to add
         * @param val the value to add
         * @return this
         */
        template<class U>
        basic_csv<T> &push(const U &val) {
            return this->template operator<<(val);
        }

        /**
         * Push a value to this
         *
         * @tparam U the type of the value to add
         * @param val the value to add
         * @return this
         */
        template<class U>
        basic_csv<T> &operator+=(const U &val) {
            return this->push(val);
        }

        /**
         * Add a value to a copy of this
         *
         * @tparam U the type of the value to add
         * @param val the value to add
         * @return this with the value added
         */
        template<class U>
        CSV_NODISCARD basic_csv<T> operator+(const U &val) const {
            basic_csv<T> res(*this);
            res.push(val);

            return res;
        }

        /**
         * Operator << for the use of csv::endl
         */
        basic_csv<T> &operator<<(basic_csv<T> &(*val)(basic_csv<T> &)) {
            return val(*this);
        }

        /**
         * End the current line
         *
         * @return this
         */
        basic_csv<T> &endline() {
            rows.emplace_back(nullptr);
            return *this;
        }

        /**
         * Convert this to a string
         *
         * @return this as a string
         */
        CSV_NODISCARD T to_string() const {
            if constexpr (util::is_u8_string_v<T>) {
                return to_string_impl<std::stringstream>();
            } else if constexpr (util::is_u16_string_v<T>) {
                return to_string_impl<std::wstringstream>();
            }
        }

        /**
         * Convert this to a utf-8 string
         *
         * @return this as a utf-8 string
         */
        CSV_NODISCARD std::string to_u8string() const {
            if constexpr (util::is_u8_string_v<T>) {
                return this->to_string();
            } else if constexpr (util::is_u16_string_v<T>) {
                return util::wstring_to_string(this->to_string());
            }
        }

        /**
         * Convert this to a utf-16 string
         *
         * @return this as a utf-16 string
         */
        CSV_NODISCARD std::wstring to_u16string() const {
            if constexpr (util::is_u8_string_v<T>) {
                return util::string_to_wstring(this->to_string());
            } else if constexpr (util::is_u16_string_v<T>) {
                return this->to_string();
            }
        }

        /**
         * Get the begin iterator
         *
         * @return the begin iterator
         */
        typename std::vector<csvrow<T>>::iterator begin() noexcept {
            return rows.begin();
        }

        /**
         * Get the begin const iterator
         *
         * @return the begin const iterator
         */
        CSV_NODISCARD typename std::vector<csvrow<T>>::const_iterator begin() const noexcept {
            return rows.begin();
        }

        /**
         * Get the end iterator
         *
         * @return the end iterator
         */
        typename std::vector<csvrow<T>>::iterator end() noexcept {
            return rows.end();
        }

        /**
         * Get the end const iterator
         *
         * @return the end const iterator
         */
        CSV_NODISCARD typename std::vector<csvrow<T>>::const_iterator end() const noexcept {
            return rows.end();
        }

        /**
         * Get the number of rows of this csv object
         *
         * @return the number of rows
         */
        CSV_NODISCARD size_t size() const {
            return rows.size();
        }

        /**
         * Check if this csv object is empty
         *
         * @return true if this is empty
         */
        CSV_NODISCARD bool empty() const {
            return rows.empty();
        }

        /**
         * Remove a row at an index
         *
         * @param index the index if the row to delete
         */
        void remove(size_t index) {
            this->rows.erase(this->rows.begin() + index);
        }

        /**
         * Clear this
         */
        void clear() noexcept {
            rows.clear();
        }

        /**
         * Get the number of columns in this object
         *
         * @return the number of rows
         */
        CSV_NODISCARD uint64_t numElements() const {
            uint64_t size = 0;
            for (const csvrow<T> &row : rows) {
                size += row.size();
            }

            return size;
        }

        /**
         * Friend operator << for output streams
         *
         * @param ostream the stream to write to
         * @param csv the object to write to ostream
         * @return the stream
         */
        friend std::ostream &operator<<(std::ostream &ostream, const basic_csv<T> &csv) {
            if constexpr (util::is_u8_string_v<T>) {
                ostream << csv.to_string();
            } else if constexpr (util::is_u16_string_v<T>) {
                ostream << util::wstring_to_string(csv.to_string());
            }

            return ostream;
        }

        /**
         * Friend operator << for output streams
         *
         * @param ostream the stream to write to
         * @param csv the object to write to ostream
         * @return the stream
         */
        friend std::wostream &operator<<(std::wostream &ostream, const basic_csv<T> &csv) {
            if constexpr (util::is_u16_string_v<T>) {
                ostream << csv.to_string();
            } else if constexpr (util::is_u8_string_v<T>) {
                ostream << util::string_to_wstring(csv.to_string());
            }

            return ostream;
        }

        /**
         * Friend operator >> for input streams
         *
         * @param istream the stream to read from
         * @param csv the object to read from istream
         * @return the stream
         */
        friend std::istream &operator>>(std::istream &istream, basic_csv<T> &csv) {
            std::string res(std::istreambuf_iterator<char>(istream), {});

            if constexpr (util::is_u8_string_v<T>) {
                csv << basic_csv::parse(res, csv.separator);
            } else if constexpr (util::is_u16_string_v<T>) {
                csv << basic_csv::parse(util::string_to_wstring(res), csv.separator);
            }

            return istream;
        }

        /**
         * Friend operator >> for input streams
         *
         * @param istream the stream to read from
         * @param csv the object to read from istream
         * @return the stream
         */
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
        /**
         * Get the current row.
         * Creates if if rows is empty.
         *
         * @return the current row
         */
        csvrow<T> &get_current() {
            if (rows.empty()) {
                rows.emplace_back(nullptr);
            }

            return rows[rows.size() - 1];
        }

        /**
         * The to string implementation
         *
         * @tparam U the string stream to use
         * @return this as a string
         */
        template<class U>
        CSV_NODISCARD T to_string_impl() const {
            U ss;
            for (size_t i = 0; i < rows.size(); i++) {
                if (i > 0) {
                    ss << std::endl;
                }

                for (const csvrowcolumn<T> &col : this->rows[i]) {
                    ss << col.rawValue() << separator;
                }
            }

            return ss.str();
        }

        /**
         * The parse implementation
         *
         * @param value the string value to parse
         */
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

        // The rows in this csv object
        std::vector<csvrow<T>> rows;
        // The separator to use
        char separator;
    };

    template<class T>
    class basic_csv_file {
    private:
        // The string type
        using string_t = std::basic_string<T, std::char_traits<T>, std::allocator<T>>;

        // The stream type
        using stream_t = std::basic_fstream<T, std::char_traits<T>>;

        // The cache iterator
        using cache_iterator = typename std::map<uint64_t, string_t>::const_iterator;
    public:
        static_assert(util::is_any_of_v<T, char, wchar_t>, "T must be one of [char, wchar_t]");

        /**
         * A const csv file row
         */
        class const_csv_file_row : public const_csvrow<string_t> {
        public:
            /**
             * Create a const csv file row
             *
             * @param data the data to use
             */
            explicit const_csv_file_row(const csvrow<string_t> &data) : const_csvrow<string_t>(data) {}
        };

        /**
         * A csv file tow
         */
        class csv_file_row : public csvrow<string_t> {
        public:
            /**
             * Create a csv file row
             *
             * @param file the file
             * @param data the data to use
             * @param line the line of the row in the file
             */
            csv_file_row(basic_csv_file &file, const csvrow<string_t> &data, int64_t line) : csvrow<string_t>(data),
                                                                                             line(line), file(file) {}

            /**
             * The csv file row destructor. Saves the line.
             */
            ~csv_file_row() {
                file.writeToFile(this->to_string(file.separator), line);
            }

        private:
            // The line in the file
            int64_t line;
            // The csv file reference
            basic_csv_file &file;
        };

        /**
         * Create a csv file
         *
         * @param path the path to the file
         * @param maxCached the number of cached elements
         * @param separator the separator to use
         */
        explicit basic_csv_file(const string_t &path, size_t maxCached = 100, char separator = ';') : separator(
                separator), path(path), cache(), maxCached(maxCached) {
            // Assign the current line to the index of the last line in the file
            currentLine = getLastFileLineIndex();
        }

        /**
         * Assign a csv object to this
         *
         * @param csv the csv object to assign
         * @return this
         */
        basic_csv_file &operator=(const basic_csv<string_t> &csv) {
            clear();
            return this->push(csv);
        }

        /**
         * Write a char array to the file
         *
         * @param val the array to write
         * @return this
         */
        basic_csv_file &operator<<(const char *val) {
            csvrow<string_t> row = getCurrentLine();

            row << csvrowcolumn<string_t>(val);
            writeToFile(row.to_string(separator), currentLine);

            return *this;
        }

        /**
         * Write a wide char array to this.
         * Only available if T = std::wstring.
         *
         * @param val the array to write
         * @return this
         */
        CSV_REQUIRES(T, std::wstring)
        basic_csv_file &operator<<(const wchar_t *val) {
            csvrow<string_t> row = getCurrentLine();

            row << csvrowcolumn<string_t>(val);
            writeToFile(row.to_string(separator), currentLine);

            return *this;
        }

        /**
         * Write a value to the file
         *
         * @tparam U the type of the value to write
         * @param val the value to write
         * @return this
         */
        template<class U>
        basic_csv_file &operator<<(const U &val) {
            csvrow<string_t> row = getCurrentLine();

            row << csvrowcolumn<string_t>(val);
            writeToFile(row.to_string(separator), currentLine);

            return *this;
        }

        /**
         * Write a csv object to the file
         *
         * @param el the object to write
         * @return this
         */
        basic_csv_file &operator<<(const basic_csv<string_t> &el) {
            csvrow<string_t> line = getCurrentLine();

            // Assign el to csv and add el[0] to the existing
            // line and push all of that into csv[o]
            basic_csv<string_t> csv = el;
            csv[0] = line + el[0];
            cache.insert_or_assign(currentLine, csv.to_string());

            // Flush and reassign currentLine
            flush();
            currentLine = getLastFileLineIndex();

            return *this;
        }

        /**
         * Operator << for basic_csv_file::endl
         */
        basic_csv_file<T> &operator<<(basic_csv_file<T> &(*val)(basic_csv_file<T> &)) {
            return val(*this);
        }

        /**
         * Friend operator >> for writing to a csv object
         *
         * @param file the file to read from
         * @param csv the csv object to write to
         * @return the csv object
         */
        friend basic_csv<string_t> &operator>>(basic_csv_file<T> &file, basic_csv<string_t> &csv) {
            file.flush();

            stream_t in = file.getStream(std::ios::in);
            in >> csv;
            in.close();

            return csv;
        }

        /**
         * Write a value to the file
         *
         * @tparam U the type of the value to write
         * @param val the value to write
         * @return this
         */
        template<class U>
        basic_csv_file &push(const U &val) {
            return this->operator<<(val);
        }

        /**
         * Get the row at an index.
         * Const-qualified, so throws an exception if the row does not exist.
         *
         * @param line the zero-based index of the row
         * @return the const row
         */
        const_csv_file_row at(size_t line) const {
            if (line > getMaxLineIndex()) {
                throw std::runtime_error("The requested line index does not exist");
            }

            return const_csv_file_row(getLine(line));
        }

        /**
         * Get the row at an index.
         * Creates the row if it does not exist.
         *
         * @param line the zero-based index of the row
         * @return the row
         */
        csv_file_row at(size_t line) {
            return csv_file_row(*this, getOrCreateLine(line), line);
        }

        /**
         * Get the row at an index.
         * Const-qualified, so throws an exception if the row does not exist.
         *
         * @param line the zero-based index of the row
         * @return the const row
         */
        const_csv_file_row operator[](size_t line) const {
            return this->at(line);
        }

        /**
         * Get the row at an index.
         * Creates the row if it does not exist.
         *
         * @param line the zero-based index of the row
         * @return the row
         */
        csv_file_row operator[](size_t line) {
            return this->at(line);
        }

        /**
         * Convert this to an csv object
         *
         * @return this as an csv object
         */
        basic_csv<string_t> to_basic_csv() {
            basic_csv<string_t> csv;
            *this >> csv;

            return csv;
        }

        /**
         * Add a new line to this
         *
         * @return this
         */
        basic_csv_file<T> &endline() {
            currentLine++;
            return *this;
        }

        /**
         * Get the number of rows in the file
         *
         * @return the number if rows
         */
        CSV_NODISCARD uint64_t size() const {
            return getMaxLineIndex() + 1;
        }

        /**
         * Clear the cache and delete the file
         */
        void clear() {
            cache.clear();
            std::remove(util::string_as<std::string>(path).c_str());
        }

        /**
         * Write the cache to the file if it isn't empty
         */
        void flush() {
            // If the cache isn't empty, write it to the file
            if (!cache.empty() || getLastFileLineIndex() < currentLine) {
                writeCacheToFile();
            }
        }

        /**
         * Flush the file on destruction
         */
        ~basic_csv_file() {
            flush();
        }

        /**
         * End line operator
         *
         * @param c the file to append a new line to
         * @return the file
         */
        static basic_csv_file<T> &endl(basic_csv_file<T> &c) {
            c.endline();
            return c;
        }

    private:
        /**
         * Write a row to the file.
         * Replaces any other values on the same line.
         *
         * @param row the row to write
         * @param line the line of the row to write
         */
        void writeToFile(const string_t &row, uint64_t line) {
            cache.insert_or_assign(line, row);

            // If the cache size is greater than or equal to
            // the max size, write the cache to the file
            if (cache.size() >= maxCached) {
                writeCacheToFile();
            }
        }

        /**
         * Get or create a row.
         * Basically creates a row if it doesn't
         * exist or returns the existing one.
         *
         * @param line the zero-based index of the line to get
         * @return the created or retrieved row
         */
        csvrow<string_t> getOrCreateLine(uint64_t line) {
            if (line <= getMaxLineIndex()) {
                return getLine(line);
            } else {
                // If line is greater than currentLine,
                // set currentLine to line
                if (line > currentLine) currentLine = line;
                cache.insert_or_assign(line, string_t());

                return csvrow<string_t>(nullptr);
            }
        }

        /**
         * Get a row at an index.
         * Returns an empty row if the row with that index does not exist.
         *
         * @param line the zero-based index of the line to get
         * @return the retrieved row
         */
        CSV_NODISCARD csvrow<string_t> getLine(uint64_t line) const {
            // Return an empty row if line does not exist
            if (line > getMaxLineIndex()) {
                return csvrow<string_t>(nullptr);
            }

            // Check if the line is in the cache
            const cache_iterator it = cache.find(line);
            if (it == cache.end()) {
                // Get a stream and navigate to the line
                stream_t in = getStream(std::ios::in);
                gotoLine(in, line);

                // Read the value of the line
                string_t value;
                std::getline(in, value);
                in.close();

                // Return the value
                if (value.empty()) {
                    return csvrow<string_t>(nullptr);
                } else {
                    return csvrow<string_t>::parse(value, separator);
                }
            } else {
                // Return the cached value
                return csvrow<string_t>::parse(it->second, separator);
            }
        }

        /**
         * Get the row stored in the current line
         *
         * @return the current row
         */
        CSV_NODISCARD csvrow<string_t> getCurrentLine() const {
            return getLine(currentLine);
        }

        /**
         * Get a stream to the csv file
         *
         * @param openMode the open mode flags
         * @return the created stream
         */
        CSV_NODISCARD stream_t getStream(std::ios_base::openmode openMode) const {
            return stream_t(path, openMode);
        }

        /**
         * Get the zero-based index of the last line in the file
         *
         * @return the last index
         */
        CSV_NODISCARD uint64_t getLastFileLineIndex() const {
            // Get the stream to the file and count the lines
            stream_t in = getStream(std::ios::in);
            uint64_t lines = std::count(std::istreambuf_iterator<T>(in), std::istreambuf_iterator<T>(), '\n');
            in.close();

            return lines;
        }

        /**
         * Get the highest stored zero-based index in the file or cache
         *
         * @return the index of the last line in the file or cache
         */
        CSV_NODISCARD uint64_t getMaxLineIndex() const {
            const uint64_t fLines = getLastFileLineIndex();

            // IF the cache isn't empty get the highest number
            // in the cache and compare it to fLine. Return whatever is larger.
            if (cache.empty()) {
                return std::max(fLines, currentLine);
            } else {
                return std::max(std::max(fLines, cache.rbegin()->first), currentLine);
            }
        }

        /**
         * Get the path to the temporary file
         *
         * @tparam U the type of the path
         * @return the path as U
         */
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

        /**
         * Write the cache to the csv file
         */
        void writeCacheToFile() {
            // Delete the tmp file and get the streams
            std::remove(getTmpFile<std::string>().c_str());
            stream_t out(getTmpFile(), std::ios::out | std::ios::app);
            stream_t in = getStream(std::ios::in);

            // The current line index
            uint64_t i = 0;
            // The content of the current line in the file
            string_t current;
            // Go to the beginning of the file
            in.seekg(std::ios::beg);
            while (std::getline(in, current)) {
                // Prepend a new line if the current line is not zero
                if (i > 0) {
                    out << std::endl;
                }

                // Check if the cache has a value for the current line.
                // IF so, write that to the tmp file instead of the original value.
                // If not, write the original value to the tmp file
                const cache_iterator it = cache.find(i++);
                if (it == cache.end()) {
                    out << current;
                } else {
                    out << it->second;
                    cache.erase(it);
                }
            }

            // Close the input stream
            in.close();

            // Write all values that were not already written to the file into the file
            if (!cache.empty()) {
                // Get the highest line number
                const uint64_t max = getMaxLineIndex();
                for (; i <= max; i++) {
                    // Prepend a new line if the current line is not zero
                    if (i > 0) {
                        out << std::endl;
                    }

                    // Write the line to the file if a value for it exists in the cache
                    const cache_iterator it = cache.find(i);
                    if (it != cache.end()) {
                        out << it->second;
                    }
                }
            } else {
                // Append new lines at the end, if required
                const uint64_t max = getMaxLineIndex();
                for (; i <= max; i++) {
                    // Prepend a new line if the current line is not zero
                    if (i > 0) {
                        out << std::endl;
                    }
                }
            }

            // Flush and close the output stream
            out.flush();
            out.close();

            // Clear the cache
            cache.clear();

            // Delete the old csv file and rename the tmp file to it
            std::remove(util::string_as<std::string>(path).c_str());
            std::rename(getTmpFile<std::string>().c_str(), util::string_as<std::string>(path).c_str());
        }

        /**
         * Move a stream to a line in a file.
         * Source: https://stackoverflow.com/a/5207600
         *
         * @param stream the stream to move
         * @param num the line to move to
         */
        static void gotoLine(stream_t &stream, uint64_t num) {
            stream.seekg(std::ios::beg);
            for (size_t i = 0; i < num; i++) {
                stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }

        // The maximum amount of cached values
        size_t maxCached;

        // The row cache
        std::map<uint64_t, string_t> cache;

        // The value separator
        char separator;

        // The path to the file
        string_t path;

        // The zero-based index of the current line
        uint64_t currentLine;
    };

    /**
     * A utf-8 csv object
     */
    using csv = basic_csv<std::string>;

    /**
     * A utf-16 csv object
     */
    using w_csv = basic_csv<std::wstring>;

    /**
     * A utf-8 csv file
     */
    using csv_file = basic_csv_file<char>;

    /**
     * A utf-16 csv file
     */
    using w_csv_file = basic_csv_file<wchar_t>;
}

// Undefine everything
#undef CSV_NODISCARD
#undef CSV_CHECK_T_SUPPORTED
#undef CSV_WINDOWS
#undef CSV_UNIX
#undef CSV_REQUIRES
#undef CSV_ENABLE_IF

#endif //MARKUSJX_CSV_HPP
