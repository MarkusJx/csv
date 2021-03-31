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

#ifndef MARKUSJX_CSV_CSV_HPP
#define MARKUSJX_CSV_CSV_HPP

#include <cstring>

#ifndef MARKUSJX_CSV_DEFINITIONS_HPP
#define MARKUSJX_CSV_DEFINITIONS_HPP

#if defined(_MSVC_LANG) || defined(__cplusplus)
#   if (defined(_MSVC_LANG) && _MSVC_LANG > 201703L) || __cplusplus > 201703L // C++20
#       define CSV_REQUIRES(type, ...) template<class = type> requires ::markusjx::util::is_any_of_v<type, __VA_ARGS__>
#   endif
#   if (defined(_MSVC_LANG) && _MSVC_LANG > 201402L) || __cplusplus > 201402L // C++17
#       ifndef CSV_REQUIRES
//          Use SFINAE to disable functions
#           define CSV_REQUIRES(type, ...) template<class _SFINAE_PARAM_ = type, class = \
                            typename std::enable_if_t<::markusjx::util::is_any_of_v<_SFINAE_PARAM_, __VA_ARGS__>, int>>
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

#ifndef MARKUSJX_CSV_SEPARATOR
#   define MARKUSJX_CSV_SEPARATOR ';'
#endif //MARKUSJX_CSV_SEPARATOR

#endif //MARKUSJX_CSV_DEFINITIONS_HPP

#ifndef MARKUSJX_CSV_ESCAPE_SEQUENCE_GENERATOR_HPP
#define MARKUSJX_CSV_ESCAPE_SEQUENCE_GENERATOR_HPP

#include <cstring>
#include <vector>

#ifndef MARKUSJX_CSV_EXCEPTIONS_HPP
#define MARKUSJX_CSV_EXCEPTIONS_HPP

#include <stdexcept>

namespace markusjx::exceptions {
    /**
     * A generic exception
     */
    class exception : public std::runtime_error {
    public:
        /**
         * Get the exception type
         *
         * @return the exception type
         */
        CSV_NODISCARD const char *getType() const noexcept {
            return type;
        }

    protected:
        /**
         * Create an exception
         *
         * @param type the exception type
         * @param msg the error message
         */
        exception(const char *type, const std::string &msg) : std::runtime_error(msg), type(type) {}

        // The exception type
        const char *type;
    };

    /**
     * A parse error
     */
    class parse_error : public exception {
    public:
        /**
         * Create a parse error
         *
         * @param msg the error message
         */
        explicit parse_error(const std::string &msg) : exception("ParseError", msg) {}
    };

    /**
     * A conversion error
     */
    class conversion_error : public exception {
    public:
        /**
         * Create a conversion error
         *
         * @param msg the error message
         */
        explicit conversion_error(const std::string &msg) : exception("ConversionError", msg) {}
    };

    /**
     * An index out of range error
     */
    class index_out_of_range_error : public exception {
    public:
        /**
         * Create an index out of range error
         *
         * @param msg the error message
         */
        explicit index_out_of_range_error(const std::string &msg) : exception("IndexOutOfRangeError", msg) {}
    };
} //namespace markusjx::exceptions

#endif //MARKUSJX_CSV_EXCEPTIONS_HPP

#ifndef MARKUSJX_CSV_UTIL_HPP
#define MARKUSJX_CSV_UTIL_HPP

#include <cstdlib>
#include <cstring>
#include <type_traits>

namespace markusjx::util {
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
     * An alias for std::basic_string
     *
     * @tparam T the type of the string
     */
    template<class T>
    using std_basic_string = std::basic_string<T, std::char_traits<T>, std::allocator<T>>;

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
            throw exceptions::conversion_error("Could not fully convert the value");
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
            throw exceptions::conversion_error("Could not convert the value");
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
            throw exceptions::conversion_error("Could not convert the string");
        }
#elif defined(CSV_UNIX)
        size_t written = wcstombs(out.data(), in.c_str(), in.size());
        if (written == static_cast<size_t>(-1)) {
            throw exceptions::conversion_error("Could not convert the string");
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
            throw exceptions::conversion_error("Could not create the string");
        }
#elif defined(CSV_UNIX)
        size_t written = mbstowcs(out.data(), in.c_str(), in.size());
        if (written == static_cast<size_t>(-1)) {
            throw exceptions::conversion_error("Could not convert the string");
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
} //namespace markusjx::util

#endif //MARKUSJX_CSV_UTIL_HPP

namespace markusjx::util {
    /**
     * The default escape sequence generator.
     * This implementation enforces the rules
     * defined in RFC 4180.
     *
     * @tparam T the string type
     * @tparam Sep the separator to use
     * @tparam C the character type
     */
    template<class T, char Sep = ';', class C = typename T::value_type>
    class escape_sequence_generator {
    public:
        CSV_CHECK_T_SUPPORTED

        /**
         * Escape a character
         *
         * @param character the character to escape
         * @return the escaped character string
         */
        CSV_NODISCARD virtual T escape_character(C character) const {
            switch (character) {
                case '\"':
                    return string_as<T>(std::string("\"\""));
                default:
                    return T(1, character);
            }
        }

        /**
         * Escape a string
         *
         * @param str the string to escape
         * @param delimiter the delimiter used in the csv file
         * @return the escaped string
         */
        CSV_NODISCARD virtual T escape_string(const T &str) const {
            // Prepend and append a double quote to the
            // string if it contains a new line, a double
            // quote or a separator (RFC 4180 section 2.6)
            if (str.find('\n') != T::npos || str.find('\"') != T::npos || str.find(Sep) != T::npos) {
                T res;

                res += '\"';
                for (const C character : str) {
                    res.append(escape_character(character));
                }
                res += '\"';

                return res;
            } else {
                return str;
            }
        }

        /**
         * Un-escape a character
         *
         * @param character the character to un-escape
         * @param converted will be set to true if the character could be un-escaped
         * @return the un-escaped character
         */
        CSV_NODISCARD virtual C unescape_character(C character, bool &converted) const {
            switch (character) {
                case '\"':
                    converted = true;
                    return static_cast<C>('\"');
                default:
                    converted = false;
                    return character;
            }
        }

        /**
         * Un-escape a string
         *
         * @param toConvert the string to un-escape
         * @param only_quotes_tm whether to only remove leading and trailing quotes if present
         * @return the un-escaped string
         */
        CSV_NODISCARD virtual T unescape_string(T toConvert, bool only_quotes_tm) const {
            if (only_quotes_tm) {
                if (toConvert.size() >= 2 && toConvert[0] == '\"' && toConvert[toConvert.size() - 1] == '\"') {
                    return toConvert.substr(1, toConvert.size() - 2);
                } else {
                    return toConvert;
                }
            } else {
                // Create the result string
                T res;

                if (toConvert.size() >= 2 && toConvert[0] == '\"' && toConvert[toConvert.size() - 1] == '\"') {
                    toConvert = toConvert.substr(1, toConvert.size() - 2);
                }

                // Iterate over the string to convert.
                // Use ptrdiff_t as type as it is the signed counterpart to size_t.
                for (ptrdiff_t i = 0; i < static_cast<signed>(toConvert.size()); i++) {
                    // Only continue if the current character is a double quote
                    // and i + 1 is smaller than the size of toConvert
                    if (toConvert[i] == '\"' && static_cast<size_t>(i + 1) < toConvert.size()) {
                        bool wasChanged;
                        const C newVal = unescape_character(toConvert[i + 1], wasChanged);

                        // Append the un-escaped value to the result
                        // if the character was un-escapable
                        if (wasChanged) {
                            res += newVal;
                            i++;
                            continue;
                        }
                    }

                    // If continue wasn't called, just add the
                    // current char to the result string
                    res += toConvert[i];
                }

                return res;
            }
        }

        /**
         * Find a delimiter in a string.
         * Returns T::npos if the delimiter was not found.
         *
         * @param str the string to find the position of the next delimiter in
         * @param offset the offset to start with
         * @param delimiter the delimiter to search for
         * @return the position of the delimiter in str
         */
        CSV_NODISCARD virtual size_t find(const T &str, size_t offset, char delimiter) const {
            // The number of double quotes
            short doubleQuotes = 0;
            for (size_t pos = offset; pos < str.length(); pos++) {
                // If the current character is a
                // double quote, increase doubleQuotes
                if (str[pos] == '\"') {
                    doubleQuotes = (doubleQuotes + 1) % 2;
                } else if (str[pos] == delimiter && doubleQuotes == 0) {
                    // If the string at pos is the delimiter and the
                    // number of double quotes in the last section
                    // is even, return the position of the delimiter.
                    return pos;
                }

                // Note: A valid string must contain an even number
                // of double quotes to be properly formatted: Each
                // double quote must be escaped by another double
                // quote (RFC 4180 section 2.7) and if there are
                // double quotes in a string, the whole string should
                // be enclosed in double quotes (RFC 4180 section 2.6).
            }

            // The number of double quotes must be even, if not, the csv string is malformed
            if (doubleQuotes != 0) {
                throw exceptions::parse_error("Missing quotation mark at the end of the string");
            } else {
                return T::npos;
            }
        }

        /**
         * Split a string by a delimiter
         *
         * @param str the string to split
         * @param delimiter the delimiter to split the string by
         * @return the string parts extracted from the string
         */
        CSV_NODISCARD virtual std::vector<T> splitString(const T &str, char delimiter) const {
            // This implementation is based on this: https://stackoverflow.com/a/37454181
            std::vector<T> tokens;
            size_t pos, prev = 0;
            do {
                pos = find(str, prev, delimiter);
                if (pos == T::npos) pos = str.length();
                T token = str.substr(prev, pos - prev);
                tokens.push_back(token);
                prev = pos + 1;
            } while (pos < str.length() && prev < str.length());

            // If the string ends with the delimiter character and isn't
            // empty, add another instance of T to the result vector.
            // This is mostly in here because a line must not end with a
            // separator (RFC 4180 section 2.4). So if it ends with a
            // separator, there must be another, empty cell behind that separator.
            // However, this is not the case for new lines, as the last record in
            // the file may or may not end with a new line (RFC 4180 section 2.2).
            if (!str.empty() && str[str.size() - 1] == delimiter && delimiter != '\n') {
                tokens.push_back(T());
            }

            return tokens;
        }
    };
} //namespace markusjx::util

#endif //MARKUSJX_CSV_ESCAPE_SEQUENCE_GENERATOR_HPP

#ifndef MARKUSJX_CSV_BASIC_CSV_HPP
#define MARKUSJX_CSV_BASIC_CSV_HPP

#include <cstring>
#include <vector>
#include <ostream>

#ifndef MARKUSJX_CSV_CSV_CELL_HPP
#define MARKUSJX_CSV_CSV_CELL_HPP

#include <cstring>
#include <string>
#include <regex>

namespace markusjx {
    /**
     * A csv cell
     *
     * @tparam T the string type. Must be a std::string or std::wstring
     * @tparam Sep the separator to use
     * @tparam _escape_generator_ the escape sequence generator to use
     */
    template<class T, char Sep = ';', class _escape_generator_ = util::escape_sequence_generator<T, Sep>>
    class csv_cell {
    public:
        CSV_CHECK_T_SUPPORTED
        static_assert(std::is_base_of_v<util::escape_sequence_generator<T, Sep>, _escape_generator_>,
                      "Template parameter _escape_generator_ must extend markusjx::util::escape_sequence_generator");

        /**
         * The string type
         */
        using string_type = T;
        using char_type = typename string_type::value_type;

        /**
         * Parse a column value
         *
         * @param value the string value to parse
         * @return the parsed column
         */
        static csv_cell<T, Sep, _escape_generator_> parse(const T &value) {
            csv_cell<T, Sep, _escape_generator_> col(nullptr);
            col.value = value;

            return col;
        }

        /**
         * Create an empty column
         */
        csv_cell(std::nullptr_t) : escapeGenerator(), value() {}

        /**
         * Create a column from a string value
         *
         * @param val the value
         */
        csv_cell(const T &val) : escapeGenerator(), value(escapeGenerator.escape_string(val)) {}

        /**
         * Create a column from a string value. Only available if T = std::wstring
         *
         * @param val the string to convert
         */
        CSV_REQUIRES(T, std::wstring)
        csv_cell(const std::string &val)
                : escapeGenerator(), value(escapeGenerator.escape_string(util::string_to_wstring(val))) {}

        /**
         * Create a column from a character
         *
         * @param val the character to use
         */
        csv_cell(char val) : escapeGenerator(), value(escapeGenerator.escape_string(T(1, val))) {}

        /**
         * Create a column from a char array
         *
         * @param val the char array
         */
        csv_cell(const char *val) : escapeGenerator() {
            if constexpr (util::is_u8_string_v<T>) {
                value = escapeGenerator.escape_string(T(val));
            } else if constexpr (util::is_u16_string_v<T>) {
                value = escapeGenerator.escape_string(util::string_to_wstring(val));
            }
        }

        /**
         * Create a column from a wide character
         *
         * @param val the char to use
         */
        CSV_REQUIRES(T, std::wstring)
        csv_cell(wchar_t val) : escapeGenerator(), value(escapeGenerator.escape_string(T(1, val))) {}

        /**
         * Create a column from a wide char array
         *
         * @param val the char array
         */
        CSV_REQUIRES(T, std::wstring)
        csv_cell(const wchar_t *val) : escapeGenerator(), value(escapeGenerator.escape_string(T(val))) {}

        /**
         * Create a column from a boolean
         *
         * @param val the bool
         */
        csv_cell(bool val) {
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
        csv_cell(const U &val) {
            if constexpr (util::is_u8_string_v<T>) {
                value = std::to_string(val);
            } else if constexpr (util::is_u16_string_v<T>) {
                value = std::to_wstring(val);
            }
        }

        csv_cell &operator=(const csv_cell<T, Sep, _escape_generator_> &) = default;

        /**
         * Assign nothing to this column
         *
         * @return this
         */
        csv_cell &operator=(std::nullptr_t) {
            value = T();
            return *this;
        }

        /**
         * Assign a string value to this column
         *
         * @param val the value to assign
         * @return this
         */
        csv_cell &operator=(const T &val) {
            if constexpr (util::is_u8_string_v<T>) {
                value = escapeGenerator.escape_string(val);
            } else if constexpr (util::is_u16_string_v<T>) {
                value = escapeGenerator.escape_string(val);
            }

            return *this;
        }

        /**
         * Assign a char array to this
         *
         * @param val the array
         * @return this
         */
        csv_cell &operator=(const char *val) {
            value = escapeGenerator.escape_string(util::string_as<T>(std::string(val)));

            return *this;
        }

        /**
         * Assign a wide char array to this. Requires T to be a wide string.
         *
         * @param val the value to assign
         * @return this
         */
        CSV_REQUIRES(T, std::wstring)
        csv_cell &operator=(const wchar_t *val) {
            value = escapeGenerator.escape_string(T(val));
            return *this;
        }

        /**
         * Assign a bool to this column
         *
         * @param val the value to assign
         * @return this
         */
        csv_cell &operator=(bool val) {
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
        csv_cell &operator=(char val) {
            value = escapeGenerator.escape_string(T(1, val));
            return *this;
        }

        /**
         * Assign a wide char to this column. Only available if T = std::wstring
         *
         * @param val the wide char to assign
         * @return this
         */
        CSV_REQUIRES(T, std::wstring)
        csv_cell &operator=(wchar_t val) {
            value = escapeGenerator.escape_string(T(1, val));
            return *this;
        }

        /**
         * Assign a std::string to this column. Only available if T = std::wstring
         *
         * @param val the wide string to assign
         * @return this
         */
        CSV_REQUIRES(T, std::wstring)
        csv_cell &operator=(const std::string &val) {
            value = escapeGenerator.escape_string(util::string_to_wstring(val));
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
        csv_cell &operator=(const U &val) {
            if constexpr (util::is_u8_string_v<T>) {
                value = std::to_string(val);
            } else if constexpr (util::is_u16_string_v<T>) {
                value = std::to_wstring(val);
            }

            return *this;
        }

        /**
         * Get the character at an index
         *
         * @param index the index of the character
         * @return the character reference
         */
        char_type &at(size_t index) {
            return this->as<T>().at(index);
        }

        /**
         * Get the character at an index
         *
         * @param index the index of the character
         * @return the character reference
         */
        CSV_NODISCARD const char_type &at(size_t index) const {
            return this->as<T>().at(index);
        }

        /**
         * Get the character at an index
         *
         * @param index the index of the character
         * @return the character reference
         */
        char_type &operator[](size_t index) {
            return this->at(index);
        }

        /**
         * Get the character at an index
         *
         * @param index the index of the character
         * @return the character reference
         */
        CSV_NODISCARD const char_type &operator[](size_t index) const {
            return this->at(index);
        }

        /**
         * Set the raw value
         *
         * @param val the new raw value
         */
        void setRawValue(const T &val) {
            this->value = val;
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
            return escapeGenerator.unescape_string(value, false);
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
                throw exceptions::conversion_error("The value is not a character");
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
                throw exceptions::conversion_error("The value is not a character");
            }
        }

        /**
         * Get this as an integer
         *
         * @return the integer
         */
        CSV_NODISCARD operator int() const {
            if (isNumber()) {
                return util::stringToNumber<int>(escapeGenerator.unescape_string(value, true), std::stoi);
            } else {
                throw exceptions::conversion_error("The value is not a number");
            }
        }

        /**
         * Get this as a long
         *
         * @return this as a long value
         */
        CSV_NODISCARD operator long() const {
            if (isNumber()) {
                return util::stringToNumber<long>(escapeGenerator.unescape_string(value, true), std::stol);
            } else {
                throw exceptions::conversion_error("The value is not a number");
            }
        }

        /**
         * Get this as a unsigned long
         *
         * @return this as a unsigned long
         */
        CSV_NODISCARD operator unsigned long() const {
            if (isNumber()) {
                return util::stringToNumber<unsigned long>(escapeGenerator.unescape_string(value, true), std::stoul);
            } else {
                throw exceptions::conversion_error("The value is not a number");
            }
        }

        /**
         * Get this as a long long
         *
         * @return this as a long long
         */
        CSV_NODISCARD operator long long() const {
            if (isNumber()) {
                return util::stringToNumber<long long>(escapeGenerator.unescape_string(value, true), std::stoll);
            } else {
                throw exceptions::conversion_error("The value is not a number");
            }
        }

        /**
         * Get this as a unsigned long long
         *
         * @return this as a unsigned long long
         */
        CSV_NODISCARD operator unsigned long long() const {
            if (isNumber()) {
                return util::stringToNumber<unsigned long long>(escapeGenerator.unescape_string(value, true),
                                                                std::stoull);
            } else {
                throw exceptions::conversion_error("The value is not a number");
            }
        }

        /**
         * Get this as a double
         *
         * @return this as a double
         */
        CSV_NODISCARD operator double() const {
            if (isNumber()) {
                return util::stringToNumberAlt<double>(escapeGenerator.unescape_string(value, true), std::stod);
            } else {
                throw exceptions::conversion_error("The value is not a number");
            }
        }

        /**
         * Get this as a long double
         *
         * @return this as a long double
         */
        CSV_NODISCARD operator long double() const {
            if (isNumber()) {
                return util::stringToNumberAlt<long double>(escapeGenerator.unescape_string(value, true), std::stold);
            } else {
                throw exceptions::conversion_error("The value is not a number");
            }
        }

        /**
         * Get this as a float
         *
         * @return this as a float
         */
        CSV_NODISCARD operator float() const {
            if (isNumber()) {
                return util::stringToNumberAlt<float>(escapeGenerator.unescape_string(value, true), std::stof);
            } else {
                throw exceptions::conversion_error("The value is not a number");
            }
        }

        /**
         * Get this as a bool
         *
         * @return this as a bool
         */
        CSV_NODISCARD operator bool() const {
            if constexpr (util::is_u8_string_v<T>) {
                return op_bool_impl(escapeGenerator.unescape_string(value, true), "true", "false");
            } else if constexpr (util::is_u16_string_v<T>) {
                return op_bool_impl(escapeGenerator.unescape_string(value, true), L"true", L"false");
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
        CSV_NODISCARD bool operator==(const csv_cell<T, Sep, _escape_generator_> &other) const {
            if (this == &other) {
                return true;
            } else {
                return this->operator T() == other.operator T();
            }
        }

        /**
         * Operator unequals with another column
         *
         * @param other the column to compare with
         * @return true if the columns do not match
         */
        CSV_NODISCARD bool operator!=(const csv_cell<T, Sep, _escape_generator_> &other) const {
            if (this == &other) {
                return false;
            } else {
                return this->operator T() != other.operator T();
            }
        }

        /**
         * Operator smaller than with another column
         *
         * @param other the column to compare with
         * @return true if this is smaller than other
         */
        CSV_NODISCARD bool operator<(const csv_cell<T, Sep, _escape_generator_> &other) const {
            if (this->isFloatingPoint() && other.isFloatingPoint()) {
                return this->as<long double>() < other.as<long double>();
            } else if (this->isFloatingPoint() && other.isDecimal()) {
                return this->as<long double>() < other.as<long long>();
            } else if (this->isDecimal() && other.isFloatingPoint()) {
                return this->as<long long>() < other.as<long double>();
            } else if (this->isDecimal() && other.isDecimal()) {
                return this->as<long long>() < other.as<long long>();
            } else {
                return this->operator T() < other.operator T();
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
        CSV_NODISCARD bool operator<=(const csv_cell<T, Sep, _escape_generator_> &other) const {
            if (this->isFloatingPoint() && other.isFloatingPoint()) {
                return this->as<long double>() <= other.as<long double>();
            } else if (this->isFloatingPoint() && other.isDecimal()) {
                return this->as<long double>() <= other.as<long long>();
            } else if (this->isDecimal() && other.isFloatingPoint()) {
                return this->as<long long>() <= other.as<long double>();
            } else if (this->isDecimal() && other.isDecimal()) {
                return this->as<long long>() <= other.as<long long>();
            } else {
                return this->operator T() <= other.operator T();
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
        CSV_NODISCARD bool operator>(const csv_cell<T, Sep, _escape_generator_> &other) const {
            if (this->isFloatingPoint() && other.isFloatingPoint()) {
                return this->as<long double>() > other.as<long double>();
            } else if (this->isFloatingPoint() && other.isDecimal()) {
                return this->as<long double>() > other.as<long long>();
            } else if (this->isDecimal() && other.isFloatingPoint()) {
                return this->as<long long>() > other.as<long double>();
            } else if (this->isDecimal() && other.isDecimal()) {
                return this->as<long long>() > other.as<long long>();
            } else {
                return this->operator T() > other.operator T();
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
        CSV_NODISCARD bool operator>=(const csv_cell<T, Sep, _escape_generator_> &other) const {
            if (this->isFloatingPoint() && other.isFloatingPoint()) {
                return this->as<long double>() >= other.as<long double>();
            } else if (this->isFloatingPoint() && other.isDecimal()) {
                return this->as<long double>() >= other.as<long long>();
            } else if (this->isDecimal() && other.isFloatingPoint()) {
                return this->as<long long>() >= other.as<long double>();
            } else if (this->isDecimal() && other.isDecimal()) {
                return this->as<long long>() >= other.as<long long>();
            } else {
                return this->operator T() >= other.operator T();
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
        csv_cell &operator++() {
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
        csv_cell operator++(int) {
            csv_cell<T, Sep, _escape_generator_> old = *this;
            this->operator++();

            return old;
        }

        /**
         * Prefix decrement operator
         *
         * @return this after its value was decreased
         */
        csv_cell &operator--() {
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
        csv_cell operator--(int) {
            csv_cell<T, Sep, _escape_generator_> old = *this;
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
        CSV_NODISCARD csv_cell operator+(const csv_cell<T, Sep, _escape_generator_> &val) const {
            if (this->isFloatingPoint() || val.isFloatingPoint()) {
                return csv_cell(this->as<long double>() + val.as<long double>());
            } else if (this->isNumber() && val.isNumber()) {
                return csv_cell(this->as<long long>() + val.as<long long>());
            } else {
                return csv_cell(this->as<T>() + val.as<T>());
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
        CSV_NODISCARD csv_cell operator+(const U &val) const {
            return csv_cell(this->as<U>() + val);
        }

        /**
         * Operator add assign.
         *
         * @tparam U the type of the value to add
         * @param val the value to add
         * @return this
         */
        template<class U>
        csv_cell &operator+=(const U &val) {
            return this->operator=(this->operator+(val));
        }

        /**
         * Operator minus.
         * This's value must be a number in order for this to work.
         *
         * @param val the column to subtract
         * @return this with the column subtracted
         */
        CSV_NODISCARD csv_cell operator-(const csv_cell<T, Sep, _escape_generator_> &val) const {
            if (this->isFloatingPoint() || val.isFloatingPoint()) {
                return csv_cell(this->as<long double>() - val.as<long double>());
            } else if (this->isNumber() && val.isNumber()) {
                return csv_cell(this->as<long long>() - val.as<long long>());
            } else {
                throw exceptions::conversion_error("The value is not a number");
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
        CSV_NODISCARD csv_cell operator-(const U &val) const {
            return csv_cell(this->as<U>() - val);
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
        csv_cell &operator-=(const U &val) {
            return this->operator=(this->operator-(val));
        }

        /**
         * Operator multiply.
         * This's value must be a number in order for this to work.
         *
         * @param val the column to multiply this with
         * @return this with val multiplied
         */
        CSV_NODISCARD csv_cell operator*(const csv_cell<T, Sep, _escape_generator_> &val) const {
            if (this->isFloatingPoint() || val.isFloatingPoint()) {
                return csv_cell(this->as<long double>() * val.as<long double>());
            } else if (this->isNumber() && val.isNumber()) {
                return csv_cell(this->as<long long>() * val.as<long long>());
            } else {
                throw exceptions::conversion_error("The value is not a number");
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
        CSV_NODISCARD csv_cell operator*(const U &val) const {
            return csv_cell(this->as<U>() * val);
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
        csv_cell &operator*=(const U &val) {
            return this->operator=(this->operator*(val));
        }

        /**
         * Operator divide.
         * This's value must be a number in order for this to work.
         *
         * @param val the column to divide this by
         * @return this divided by val
         */
        CSV_NODISCARD csv_cell operator/(const csv_cell<T, Sep, _escape_generator_> &val) const {
            if (this->isFloatingPoint() || val.isFloatingPoint()) {
                return csv_cell(this->as<long double>() / val.as<long double>());
            } else if (this->isNumber() && val.isNumber()) {
                return csv_cell(this->as<long long>() / val.as<long long>());
            } else {
                throw exceptions::conversion_error("The value is not a number");
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
        CSV_NODISCARD csv_cell operator/(const U &val) const {
            return csv_cell(this->as<U>() / val);
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
        csv_cell &operator/=(const U &val) {
            return this->operator=(this->operator/(val));
        }

        /**
         * Get the size of the string value
         *
         * @return the size of the string value
         */
        CSV_NODISCARD size_t size() const {
            return this->as<T>().size();
        }

        /**
         * Get the length of the string value
         *
         * @return the length of the string value
         */
        CSV_NODISCARD size_t length() const {
            return this->as<T>().length();
        }

        /**
         * Check if this column is empty
         *
         * @return true if this column is empty
         */
        CSV_NODISCARD bool empty() const {
            return this->as<T>().empty();
        }

        /**
         * Check if this column is a number
         *
         * @return true if this is a number
         */
        CSV_NODISCARD bool isNumber() const {
            const std::basic_regex<char_type> number_regex(util::string_as<T>(std::string("^-?[0-9]+(\\.[0-9]+)?$")));
            return std::regex_match(value, number_regex);
        }

        /**
         * Check if this column is a decimal number
         *
         * @return true if this is a decimal
         */
        CSV_NODISCARD bool isDecimal() const {
            const std::basic_regex<char_type> decimal_regex(util::string_as<T>(std::string("^-?[0-9]+$")));
            return std::regex_match(value, decimal_regex);
        }

        /**
         * Check if this column is a floating point integer
         *
         * @return true if this is a floating point integer
         */
        CSV_NODISCARD bool isFloatingPoint() const {
            const std::basic_regex<char_type> float_regex(util::string_as<T>(std::string("^-?[0-9]+\\.[0-9]+$")));
            return std::regex_match(value, float_regex);
        }

        /**
         * Check if this cell is a boolean
         *
         * @return true if this cell is a boolean
         */
        CSV_NODISCARD bool isBoolean() const {
            const std::basic_regex<char_type> bool_regex(util::string_as<T>(std::string("^(true)|(false)$")));
            return std::regex_match(value, bool_regex);
        }

        /**
         * Check if this cell is a character
         *
         * @return true if this cell is a char
         */
        CSV_NODISCARD bool isChar() const {
            return this->length() == 1;
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
                throw exceptions::conversion_error("Could not convert the value");
            }
        }

        // The escape sequence generator
        _escape_generator_ escapeGenerator;

        // The string value stored in this column
        T value;
    };
} //namespace markusjx

#endif //MARKUSJX_CSV_CSV_CELL_HPP

#ifndef MARKUSJX_CSV_CSV_ROW_HPP
#define MARKUSJX_CSV_CSV_ROW_HPP

#include <cstring>
#include <vector>

#ifndef MARKUSJX_CSV_CONST_CSV_ROW_HPP
#define MARKUSJX_CSV_CONST_CSV_ROW_HPP

#include <cstring>
#include <vector>

namespace markusjx {
    /**
     * A constant csv row
     *
     * @tparam T the type of the row
     * @tparam Sep the separator to use
     * @tparam _escape_generator_ the escape sequence generator to use
     */
    template<class T, char Sep = ';', class _escape_generator_ = util::escape_sequence_generator<T, Sep>>
    class const_csv_row {
    public:
        CSV_CHECK_T_SUPPORTED

        /**
         * The cell iterator
         */
        using cell_iterator = typename std::vector<csv_cell<T, Sep, _escape_generator_>>::const_iterator;

        /**
         * Create an empty row
         */
        const_csv_row() : cells() {}

        /**
         * Copy constructor
         *
         * @param other the object to copy
         */
        const_csv_row(const const_csv_row &other) : cells(other.cells) {}

        /**
         * Create an empty row
         */
        explicit const_csv_row(std::nullptr_t) : cells() {}

        /**
         * Create a row from a data vector
         *
         * @param data the data vector
         */
        const_csv_row(const std::vector<csv_cell<T, Sep, _escape_generator_>> &data) : cells(data) {}

        /**
         * Create a row from a data vector
         *
         * @param data the data vector
         */
        const_csv_row(std::vector<csv_cell<T, Sep, _escape_generator_>> &&data) : cells(std::move(data)) {}

        /**
         * Create a row from a data vector
         *
         * @tparam U the type of the data vector
         * @param data the data vector
         */
        template<class U>
        const_csv_row(const std::vector<U> &data) : cells(data.size()) {
            for (size_t i = 0; i < data.size(); i++) {
                cells[i] = csv_cell<T, Sep, _escape_generator_>(data[i]);
            }
        }

        /**
         * Create a row from a std::initializer_list
         *
         * @param data the std::initializer_list
         */
        const_csv_row(const std::initializer_list<csv_cell<T, Sep, _escape_generator_>> &data) : cells(data) {}

        /**
         * Create a row from a std::initializer_list
         *
         * @tparam U the type of the list
         * @param list the data list
         */
        template<class U>
        const_csv_row(const std::initializer_list<U> &list) : cells() {
            cells.reserve(list.size());
            for (const U &val: list) {
                cells.emplace_back(val);
            }
        }

        /**
         * This is const, so just don't.
         */
        const_csv_row &operator=(const const_csv_row &) = delete;

        /**
         * This is const, so just don't.
         */
        const_csv_row &operator=(const_csv_row &&) = delete;

        /**
         * Operator add
         *
         * @param other the row to add to this
         * @return this with the values of other added
         */
        const_csv_row operator+(const const_csv_row<T, Sep, _escape_generator_> &other) const {
            const_csv_row<T, Sep, _escape_generator_> res(*this);
            res.cells.insert(res.cells.end(), other.cells.begin(), other.cells.end());

            return res;
        }

        /**
         * Get a csv column at an index
         *
         * @param index the index of the column to get
         * @return the retrieved column
         */
        const csv_cell<T, Sep, _escape_generator_> &at(size_t index) const {
            return this->cells.at(index);
        }

        /**
         * Get a csv column at an index
         *
         * @param index the index of the column to get
         * @return the retrieved column
         */
        const csv_cell<T, Sep, _escape_generator_> &operator[](size_t columnIndex) const {
            return this->at(columnIndex);
        }

        /**
         * Operator equals
         *
         * @param other the row to compare this to
         * @return true if this equals other
         */
        CSV_NODISCARD bool operator==(const const_csv_row<T, Sep, _escape_generator_> &other) const {
            if (this == &other) {
                // Early return if this matches other exactly
                return true;
            }

            if (this->min_size() == other.min_size()) {
                // If the sizes match, compare the individual values of the cells
                for (size_t i = 0; i < this->min_size(); i++) {
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
        CSV_NODISCARD bool operator!=(const const_csv_row<T, Sep, _escape_generator_> &other) const {
            return !this->operator==(other);
        }

        /**
         * Get the begin iterator
         *
         * @return the begin iterator
         */
        CSV_NODISCARD virtual cell_iterator begin() const noexcept {
            return cells.begin();
        }

        /**
         * Get the end iterator
         *
         * @return the end iterator
         */
        CSV_NODISCARD virtual cell_iterator end() const noexcept {
            return cells.end();
        }

        /**
         * Get the number of cells in this row
         *
         * @return the number of cells in this row
         */
        CSV_NODISCARD size_t size() const noexcept {
            return cells.size();
        }

        /**
         * Get the minimum size of this row (excluding all empty cells at the end)
         *
         * @return the minimum size of this row
         */
        CSV_NODISCARD size_t min_size() const noexcept {
            size_t sz = cells.size();
            for (ptrdiff_t i = static_cast<signed>(cells.size()) - 1; i >= 0 && cells[i].empty(); i--) {
                sz--;
            }

            return sz;
        }

        /**
         * Check if this row is empty
         *
         * @return true if this row is empty
         */
        CSV_NODISCARD bool empty() const noexcept {
            return cells.empty();
        }

        /**
         * Convert this row to a string
         *
         * @return this as a string
         */
        CSV_NODISCARD T to_string(size_t len = 0) const {
            if constexpr (util::is_u8_string_v<T>) {
                return to_string_impl<std::stringstream>(len);
            } else if constexpr (util::is_u16_string_v<T>) {
                return to_string_impl<std::wstringstream>(len);
            }
        }

        friend std::ostream &operator<<(std::ostream &stream, const const_csv_row &row) {
            stream << row.to_string();
            return stream;
        }

    protected:
        /**
         * The to string implementation
         *
         * @tparam U the type of the string stream to use
         * @return this as a string
         */
        template<class U>
        CSV_NODISCARD T to_string_impl(size_t len) const {
            const size_t max = std::max(this->min_size(), len);
            U ss;

            // Write all values with the separator at the end to the stream
            for (size_t i = 0; i < max; i++) {
                if (i < cells.size()) {
                    ss << cells[i].rawValue();
                }

                // Append the separator to each field except
                // the last one (RFC 4180 section 2.4)
                if (i < (max - 1)) {
                    ss << Sep;
                }
            }

            return ss.str();
        }

        // The cells in this row
        std::vector<csv_cell<T, Sep, _escape_generator_>> cells;
    };
} //namespace markusjx

#endif //MARKUSJX_CSV_CONST_CSV_ROW_HPP

namespace markusjx {
    /**
     * A csv row
     *
     * @tparam T the type of the row
     * @tparam Sep the separator to use
     * @tparam _escape_generator_ the escape sequence generator to use
     */
    template<class T, char Sep = ';', class _escape_generator_ = util::escape_sequence_generator<T, Sep>>
    class csv_row : public const_csv_row<T, Sep, _escape_generator_> {
    public:
        CSV_CHECK_T_SUPPORTED

        using const_cell_iterator = typename const_csv_row<T, Sep, _escape_generator_>::cell_iterator;
        using cell_iterator = typename std::vector<csv_cell<T, Sep, _escape_generator_>>::iterator;

        /**
         * Parse a row string
         *
         * @param value the string value to parse
         * @return the parsed row
         */
        static csv_row<T, Sep, _escape_generator_> parse(const T &value) {
            csv_row row(nullptr);
            _escape_generator_ escapeGenerator;

            if (!value.empty()) {
                // Write all cells in the row's column vector
                for (const T &col : escapeGenerator.splitString(value, Sep)) {
                    row << csv_cell<T, Sep, _escape_generator_>::parse(col);
                }
            }

            return row;
        }

        using const_csv_row<T, Sep, _escape_generator_>::const_csv_row;

        /**
         * Create a csv_row from a const_csv_row
         *
         * @param other the const csv row to create this from
         */
        csv_row(const const_csv_row<T, Sep, _escape_generator_> &other) : const_csv_row<T, Sep, _escape_generator_>(
                other) {}

        /**
         * Create a csv_row from a const_csv_row
         *
         * @param other the const csv row to create this from
         */
        csv_row(const_csv_row<T, Sep, _escape_generator_> &&other) : const_csv_row<T, Sep, _escape_generator_>(
                std::move(other)) {}

        /**
         * Copy constructor
         *
         * @param row the row to copy
         */
        csv_row(const csv_row<T, Sep, _escape_generator_> &row) : const_csv_row<T, Sep, _escape_generator_>(row) {}

        /**
         * Assign operator
         *
         * @param other the row to assign to this
         * @return this
         */
        csv_row &operator=(const csv_row<T, Sep, _escape_generator_> &other) {
            this->cells = other.cells;
            return *this;
        }

        /**
         * Assign a data vector to this
         *
         * @param data the vector to assign to this
         * @return this
         */
        csv_row &operator=(const std::vector<csv_cell<T, Sep, _escape_generator_>> &data) {
            this->cells = data;
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
        csv_row &operator=(const std::vector<U> &data) {
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
        csv_row &operator=(const std::initializer_list<csv_cell<T, Sep, _escape_generator_>> &data) {
            this->cells = std::vector<csv_cell<T, Sep, _escape_generator_>>(data);
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
        csv_row &operator=(const std::initializer_list<U> &data) {
            for (size_t i = 0; i < data.size(); i++) {
                this->at(i).operator=(data[i]);
            }

            return *this;
        }

        /**
         * Operator assign add
         *
         * @param other the row to add to this
         * @return this
         */
        csv_row &operator+=(const const_csv_row<T, Sep, _escape_generator_> &other) {
            for (const csv_cell<T, Sep, _escape_generator_> &col : other) {
                this->cells.push_back(col);
            }

            return *this;
        }

        /**
         * Get the column at an index
         *
         * @param index the index of the column to get
         * @return the column at index
         */
        csv_cell<T, Sep, _escape_generator_> &at(size_t index) {
            if (index >= this->cells.size()) {
                for (size_t i = this->cells.size(); i <= index; i++) {
                    this->cells.emplace_back(nullptr);
                }
            }

            return this->cells.at(index);
        }

        /**
         * Get the column at an index
         *
         * @param index the index of the column to get
         * @return the column at index
         */
        csv_cell<T, Sep, _escape_generator_> &operator[](size_t columnIndex) {
            return this->at(columnIndex);
        }

        /**
         * Operator <<
         *
         * @param col the column to add to this
         * @return this
         */
        csv_row &operator<<(const csv_cell<T, Sep, _escape_generator_> &col) {
            this->cells.push_back(col);
            return *this;
        }

        /**
         * Operator <<
         *
         * @param col the row to add to this
         * @return this
         */
        csv_row &operator<<(const const_csv_row<T, Sep, _escape_generator_> &other) {
            for (const csv_cell<T, Sep, _escape_generator_> &col : other) {
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
        csv_row &push(const U &val) {
            return this->operator<<(val);
        }

        /**
         * Get the next column.
         * Adds an empty column to this and returns it
         *
         * @return the next column
         */
        csv_cell<T, Sep, _escape_generator_> &get_next() {
            return this->cells.emplace_back(nullptr);
        }

        CSV_NODISCARD const_cell_iterator begin() const noexcept override {
            return this->cells.begin();
        }

        /**
         * Get the begin iterator
         *
         * @return the begin iterator
         */
        cell_iterator begin() noexcept {
            return this->cells.begin();
        }

        CSV_NODISCARD const_cell_iterator end() const noexcept override {
            return this->cells.end();
        }

        /**
         * Get the end iterator
         *
         * @return the end iterator
         */
        cell_iterator end() noexcept {
            return this->cells.end();
        }

        /**
         * Clear this row
         */
        void clear() noexcept {
            this->cells.clear();
        }

        /**
         * Remove a value from this row
         *
         * @param index the index of the cell to remove
         */
        void remove(size_t index) {
            this->cells.erase(this->cells.begin() + index);
        }

        /**
         * Remove all empty cells at the end of this row
         */
        void strip() {
            while (this->cells.end() > this->cells.begin() && (this->cells.end() - 1)->empty()) {
                this->cells.pop_back();
            }
        }
    };
} //namespace markusjx

#endif //MARKUSJX_CSV_CSV_ROW_HPP

#ifndef MARKUSJX_CSV_BASIC_CSV_FILE_DEF_HPP
#define MARKUSJX_CSV_BASIC_CSV_FILE_DEF_HPP

namespace markusjx {
    /**
     * A csv file
     *
     * @tparam T the char type of the file
     * @tparam Sep the separator to use
     * @tparam _escape_generator_ the escape sequence generator to use
     */
    template<class T, char Sep = ';', class _escape_generator_ = util::escape_sequence_generator<util::std_basic_string<T>, Sep>>
    class basic_csv_file;
} //namespace markusjx

#endif //MARKUSJX_CSV_BASIC_CSV_FILE_DEF_HPP

namespace markusjx {
    /**
     * A csv object
     *
     * @tparam T the string type of the object. Must be either std::string or std::wstring
     * @tparam Sep the separator to use
     * @tparam _escape_generator_ the escape sequence generator to use
     */
    template<class T, char Sep = ';', class _escape_generator_ = util::escape_sequence_generator<T, Sep>>
    class basic_csv {
    public:
        CSV_CHECK_T_SUPPORTED

        using row_iterator = typename std::vector<csv_row<T, Sep, _escape_generator_>>::iterator;
        using const_row_iterator = typename std::vector<csv_row<T, Sep, _escape_generator_>>::const_iterator;

        /**
         * Parse a csv string
         *
         * @param value the string to parse
         * @return the parsed csv object
         */
        static basic_csv<T, Sep, _escape_generator_> parse(const T &value) {
            basic_csv<T, Sep, _escape_generator_> csv;
            csv.parseImpl(value);

            return csv;
        }

        /**
         * End the line
         *
         * @param c the csv object to add a new line
         * @return the csv object
         */
        static basic_csv<T, Sep, _escape_generator_> &endl(basic_csv<T, Sep, _escape_generator_> &c) {
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
        static basic_csv_file<U, Sep, _escape_generator_> &endl(basic_csv_file<U, Sep, _escape_generator_> &f) {
            f.endline();
            return f;
        }

        /**
         * Create an csv object
         */
        explicit basic_csv() : rows() {}

        /**
         * Create an empty csv object
         */
        basic_csv(std::nullptr_t) : rows() {}

        /**
         * Create an csv object from a string
         *
         * @param value the string to parse
         */
        basic_csv(const T &value) : rows() {
            this->parseImpl(value);
        }

        /**
         * Create an csv object from a string.
         * Only available if T = std::wstring
         *
         * @param value the csv string to parse
         */
        CSV_REQUIRES(T, std::wstring)
        basic_csv(const std::string &value) : rows() {
            this->parseImpl(util::string_to_wstring(value));
        }

        /**
         * Create an csv object from a char array
         *
         * @param value the string char array to parse
         */
        basic_csv(const char *value) : rows() {
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
         */
        CSV_REQUIRES(T, std::wstring)
        basic_csv(const wchar_t *value) : rows() {
            this->parseImpl(T(value));
        }

        /**
         * Create an csv object from a data vector
         *
         * @param data the data vector
         */
        basic_csv(const std::vector<csv_row<T, Sep, _escape_generator_>> &data) : rows(data) {}

        /**
         * Create an csv object from a data vector
         *
         * @param data the data vector
         */
        basic_csv(const std::vector<csv_cell<T, Sep, _escape_generator_>> &data) : rows(
                {csv_row<T, Sep, _escape_generator_>(data)}) {}

        /**
         * Create an csv object from a std::initializer_list
         *
         * @param data the data initializer_list
         */
        basic_csv(const std::initializer_list<csv_row<T, Sep, _escape_generator_>> &data) : rows(data) {}

        /**
         * Create an csv object from a std::initializer_list
         *
         * @param data the data initializer_list
         */
        basic_csv(const std::initializer_list<csv_cell<T, Sep, _escape_generator_>> &data) : rows(
                {csv_row<T, Sep, _escape_generator_>(data)}) {}

        /**
         * Create a csv object from a csv file
         *
         * @tparam U the template type of the csv file object
         * @param file the csv file object to convert
         */
        template<class U, CSV_ENABLE_IF(util::is_any_of_v<U, char, wchar_t> &&
        std::is_same_v<std::basic_string<U, std::char_traits<U>, std::allocator<U>>, T>) >
        basic_csv(basic_csv_file<U, Sep, _escape_generator_> &file) : rows() {
            basic_csv<T, Sep, _escape_generator_> converted = file.to_basic_csv();
            rows = std::move(converted.rows);
        }

        /**
         * Assign a csv file to this csv object
         *
         * @tparam U the template type of the csv file object
         * @param file the csv file object to convert
         * @return this, after the assignment
         */
        template<class U, CSV_ENABLE_IF(util::is_any_of_v<U, char, wchar_t> &&
        std::is_same_v<std::basic_string<U, std::char_traits<U>, std::allocator<U>>, T>) >
        basic_csv &operator=(basic_csv_file<U, Sep, _escape_generator_> &file) {
            return this->operator=(std::move(file.to_basic_csv()));
        }

        /**
         * Get a row at an index.
         * Inserts the row if it doesn't exist.
         *
         * @param index the index of the row to get
         * @return the retrieved row
         */
        csv_row<T, Sep, _escape_generator_> &at(size_t index) {
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
        const csv_row<T, Sep, _escape_generator_> &at(size_t index) const {
            return rows[index];
        }

        /**
         * Get a row at an index.
         * Inserts the row if it doesn't exist.
         *
         * @param index the index of the row to get
         * @return the retrieved row
         */
        csv_row<T, Sep, _escape_generator_> &operator[](size_t index) {
            return this->at(index);
        }

        /**
         * Get a row at an index
         *
         * @param index the index of the row to get
         * @return the retrieved row
         */
        const csv_row<T, Sep, _escape_generator_> &operator[](size_t index) const {
            return this->at(index);
        }

        /**
         * Append a csv object to this
         *
         * @param other the object to add
         * @return this
         */
        basic_csv &operator<<(const basic_csv<T, Sep, _escape_generator_> &other) {
            for (const csv_row<T, Sep, _escape_generator_> &row : other) {
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
        basic_csv &operator<<(const csv_cell<T, Sep, _escape_generator_> &col) {
            get_current() << col;
            return *this;
        }

        /**
         * Append a row to this
         *
         * @param row the row to append
         * @return this
         */
        basic_csv &operator<<(const csv_row<T, Sep, _escape_generator_> &row) {
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
        basic_csv &operator<<(const U &val) {
            get_current().get_next() = val;
            return *this;
        }

        /**
         * Operator equals
         *
         * @param other the csv object to compare this with
         * @return true if the objects match
         */
        CSV_NODISCARD bool operator==(const basic_csv<T, Sep, _escape_generator_> &other) const {
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
        CSV_NODISCARD bool operator!=(const basic_csv<T, Sep, _escape_generator_> &other) const {
            return !this->operator==(other);
        }

        /**
         * Push a data vector to this
         *
         * @param data the data vector to append
         * @return this
         */
        basic_csv &push(const std::vector<csv_cell<T, Sep, _escape_generator_>> &data) {
            for (const csv_cell<T, Sep, _escape_generator_> &col : data) {
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
        basic_csv &push(const std::initializer_list<csv_cell<T, Sep, _escape_generator_>> &data) {
            for (const csv_cell<T, Sep, _escape_generator_> &col : data) {
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
        basic_csv &push(const std::vector<csv_row<T, Sep, _escape_generator_>> &data) {
            for (const csv_row<T, Sep, _escape_generator_> &col : data) {
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
        basic_csv &push(const std::initializer_list<csv_row<T, Sep, _escape_generator_>> &data) {
            for (const csv_row<T, Sep, _escape_generator_> &col : data) {
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
        basic_csv &push(const U &val) {
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
        basic_csv &operator+=(const U &val) {
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
        CSV_NODISCARD basic_csv operator+(const U &val) const {
            basic_csv<T, Sep, _escape_generator_> res(*this);
            res.push(val);

            return res;
        }

        /**
         * Operator << for the use of csv::endl
         */
        basic_csv &operator<<(basic_csv<T, Sep, _escape_generator_> &(*val)(basic_csv<T, Sep, _escape_generator_> &)) {
            return val(*this);
        }

        /**
         * End the current line
         *
         * @return this
         */
        basic_csv &endline() {
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
        row_iterator begin() noexcept {
            return rows.begin();
        }

        /**
         * Get the begin const iterator
         *
         * @return the begin const iterator
         */
        CSV_NODISCARD const_row_iterator begin() const noexcept {
            return rows.begin();
        }

        /**
         * Get the end iterator
         *
         * @return the end iterator
         */
        row_iterator end() noexcept {
            return rows.end();
        }

        /**
         * Get the end const iterator
         *
         * @return the end const iterator
         */
        CSV_NODISCARD const_row_iterator end() const noexcept {
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
         * Remove all empty cells at the end of all rows
         * and remove all empty rows at the end of this object
         */
        void strip() {
            // Strip all rows
            for (csv_row<T, Sep, _escape_generator_> &row : rows) {
                row.strip();
            }

            // Remove all empty rows from the end of the row vector
            while (rows.end() > rows.begin() && (rows.end() - 1)->empty()) {
                rows.pop_back();
            }
        }

        /**
         * Get the length of the longest row in this csv object
         *
         * @return the length of the longest row
         */
        CSV_NODISCARD size_t maxRowLength() const {
            size_t max = 0;
            for (const csv_row<T, Sep, _escape_generator_> &row : rows) {
                if (row.min_size() > max) {
                    max = row.min_size();
                }
            }

            return max;
        }

        /**
         * Get the number of columns in this object
         *
         * @return the number of rows
         */
        CSV_NODISCARD uint64_t numElements() const {
            uint64_t size = 0;
            for (const csv_row<T, Sep, _escape_generator_> &row : rows) {
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
        friend std::ostream &operator<<(std::ostream &ostream, const basic_csv<T, Sep, _escape_generator_> &csv) {
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
        friend std::wostream &operator<<(std::wostream &ostream, const basic_csv<T, Sep, _escape_generator_> &csv) {
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
        friend std::istream &operator>>(std::istream &istream, basic_csv<T, Sep, _escape_generator_> &csv) {
            std::string res(std::istreambuf_iterator<char>(istream), {});

            if constexpr (util::is_u8_string_v<T>) {
                csv << basic_csv::parse(res);
            } else if constexpr (util::is_u16_string_v<T>) {
                csv << basic_csv::parse(util::string_to_wstring(res));
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
        friend std::wistream &operator>>(std::wistream &istream, basic_csv<T, Sep, _escape_generator_> &csv) {
            std::wstring res(std::istreambuf_iterator<wchar_t>(istream), {});

            if constexpr (util::is_u8_string_v<T>) {
                csv << basic_csv::parse(util::wstring_to_string(res));
            } else if constexpr (util::is_u16_string_v<T>) {
                csv << basic_csv::parse(res);
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
        csv_row<T, Sep, _escape_generator_> &get_current() {
            // Push a new empty row to the row list if list is empty
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
            const size_t max = maxRowLength();
            U ss;

            for (size_t i = 0; i < rows.size(); i++) {
                // Prepend a new line if there was already data written
                if (i > 0) {
                    ss << std::endl;
                }

                ss << rows[i].to_string(max);
            }

            return ss.str();
        }

        /**
         * The parse implementation
         *
         * @param value the string value to parse
         */
        void parseImpl(const T &value) {
            _escape_generator_ escapeGenerator;

            // Split the string by newlines and
            // push the lines into the row vector
            for (const T &row : escapeGenerator.splitString(value, '\n')) {
                rows.push_back(csv_row<T, Sep, _escape_generator_>::parse(row));
            }
        }

        // The rows in this csv object
        std::vector<csv_row<T, Sep, _escape_generator_>> rows;
    };
} //namespace markusjx

#endif //MARKUSJX_CSV_BASIC_CSV_HPP

#ifndef MARKUSJX_CSV_BASIC_CSV_FILE_HPP
#define MARKUSJX_CSV_BASIC_CSV_FILE_HPP

#include <cstring>
#include <vector>
#include <map>
#include <fstream>

namespace markusjx {
    template<class T, char Sep, class _escape_generator_>
    class basic_csv_file {
    public:
        static_assert(util::is_any_of_v<T, char, wchar_t>, "T must be one of [char, wchar_t]");

        // The string type
        using string_type = std::basic_string<T, std::char_traits<T>, std::allocator<T>>;

        // The stream type
        using stream_type = std::basic_fstream<T, std::char_traits<T>>;

        // The cache iterators
        using cache_iterator = typename std::map<uint64_t, csv_row<string_type, Sep, _escape_generator_>>::iterator;
        using const_cache_iterator = typename std::map<uint64_t, csv_row<string_type, Sep, _escape_generator_>>::const_iterator;

        /**
         * Create a csv file
         *
         * @param path the path to the file
         * @param maxCached the number of cached elements
         */
        explicit basic_csv_file(const string_type &path, size_t maxCached = 100)
                : toDelete(), maxCached(maxCached), cache(), path(path) {
            // Assign the current line to the index of the last line in the file
            currentLine = getLastFileLineIndex();
        }

        /**
         * Assign a csv object to this
         *
         * @param csv the csv object to assign
         * @return this
         */
        basic_csv_file &operator=(const basic_csv<string_type, Sep, _escape_generator_> &csv) {
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
            csv_row<string_type, Sep, _escape_generator_> row = getCurrentLine();

            row << csv_cell<string_type, Sep, _escape_generator_>(val);
            writeToFile(row.to_string(), currentLine);

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
            csv_row<string_type, Sep, _escape_generator_> row = getCurrentLine();

            row << csv_cell<string_type, Sep, _escape_generator_>(val);
            writeToFile(row.to_string(), currentLine);

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
            csv_row<string_type, Sep, _escape_generator_> &row = getCurrentLine();
            row << csv_cell<string_type, Sep, _escape_generator_>(val);

            return *this;
        }

        /**
         * Write a csv object to the file
         *
         * @param el the object to write
         * @return this
         */
        basic_csv_file &operator<<(const basic_csv<string_type, Sep, _escape_generator_> &csv) {
            // Get the current line
            const const_csv_row<string_type, Sep, _escape_generator_> line = getCurrentLine();

            // Add a new line if the current line is not empty
            if (!line.empty()) {
                this->endline();
            }

            for (ptrdiff_t i = 0; i < static_cast<signed>(csv.size()); i++) {
                cache.insert_or_assign(currentLine, csv[i]);

                // Only increase the current line if i is
                // not the last line index in csv.
                // Remember: currentLine is also an index
                if (i < static_cast<signed>(csv.size()) - 1) {
                    currentLine++;
                }
            }

            // Flush
            flush();

            return *this;
        }

        /**
         * Operator << for basic_csv_file::endl
         */
        basic_csv_file &
        operator<<(basic_csv_file<T, Sep, _escape_generator_> &(*val)(basic_csv_file<T, Sep, _escape_generator_> &)) {
            return val(*this);
        }

        /**
         * Friend operator >> for writing to a csv object.
         * Triggers an instant rewrite of the stored date to the disk.
         *
         * @param file the file to read from
         * @param csv the csv object to write to
         * @return the csv object
         */
        friend basic_csv<string_type, Sep, _escape_generator_> &
        operator>>(basic_csv_file<T, Sep, _escape_generator_> &file,
                   basic_csv<string_type, Sep, _escape_generator_> &csv) {
            file.flush();

            stream_type in = file.getStream(std::ios::in);
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
        const_csv_row<string_type, Sep, _escape_generator_> at(uint64_t line) const {
            if (line > getMaxLineIndex()) {
                throw exceptions::index_out_of_range_error("The requested line index does not exist");
            }

            translateLine(line);
            return getLineFromFile(line);
        }

        /**
         * Get the row at an index.
         * Creates the row if it does not exist.
         *
         * @param line the zero-based index of the row
         * @return the row
         */
        csv_row<string_type, Sep, _escape_generator_> &at(uint64_t line) {
            translateLine(line);
            return getOrCreateLine(line);
        }

        /**
         * Get the row at an index.
         * Const-qualified, so throws an exception if the row does not exist.
         *
         * @param line the zero-based index of the row
         * @return the const row
         */
        const_csv_row<string_type, Sep, _escape_generator_> operator[](uint64_t line) const {
            return this->at(line);
        }

        /**
         * Get the row at an index.
         * Creates the row if it does not exist.
         *
         * @param line the zero-based index of the row
         * @return the row
         */
        csv_row<string_type, Sep, _escape_generator_> &operator[](uint64_t line) {
            return this->at(line);
        }

        /**
         * Convert this to an csv object
         *
         * @return this as an csv object
         */
        basic_csv<string_type, Sep, _escape_generator_> to_basic_csv() {
            basic_csv<string_type, Sep, _escape_generator_> csv;
            *this >> csv;

            return csv;
        }

        /**
         * Add a new line to this
         *
         * @return this
         */
        basic_csv_file &endline() {
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
         * Remove a csv row at an index
         *
         * @param index the zero-based index of the row to delete
         */
        void remove(uint64_t index) {
            if (index > getMaxLineIndex()) {
                throw exceptions::index_out_of_range_error("The requested index is out of range");
            }

            // Translate the index
            translateLine(index);

            // Erase the line from the cache if it is stored in there
            const const_cache_iterator it = cache.find(index);
            if (it != cache.end()) {
                cache.erase(it);
            }

            // Add the index to the list of lines to delete
            // and sort that list as translateLine() depends
            // on that list being sorted
            toDelete.push_back(index);
            std::sort(toDelete.begin(), toDelete.end());

            // Flush if required
            if (getCacheSize() >= maxCached) {
                flush();
            }
        }

        /**
         * Clear the cache and delete the file
         */
        void clear() {
            cache.clear();
            toDelete.clear();
            std::remove(util::string_as<std::string>(path).c_str());
        }

        /**
         * Get the length of the longest row in this csv file
         *
         * @return the length of the longest row
         */
        CSV_NODISCARD size_t maxRowLength() const {
            size_t max = 0;
            for (uint64_t i = 0; i < size(); i++) {
                size_t sz = at(i).size();
                if (sz > max) {
                    max = sz;
                }
            }

            return max;
        }

        /**
         * Write the cache to the file if it isn't empty
         */
        void flush() {
            // If the cache isn't empty, write it to the file
            if (getCacheSize() > 0 || getMaxLineIndex() < currentLine || currentLine != getLastFileLineIndex()) {
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
        static basic_csv_file &endl(basic_csv_file<T, Sep, _escape_generator_> &c) {
            c.endline();
            return c;
        }

    private:
        /**
         * Translate a line index to the actual index to use.
         * This is required as line deletions are cached and
         * not instantly written to the disk to preserve the
         * indices for the user as if the lines were deleted.
         * If we take a look at the array {0, 1, 2, 3} and
         * delete the item with index zero, the item with the
         * index one will now be the first element in the
         * array, so arr[0]. But as long as we don't actually
         * delete arr[0], object '1' will be at position one
         * but the user should think it is at position zero,
         * so if the user wants arr[0], we'll have to translate
         * that so we can return arr[1], which will become
         * arr[0] once we flush the cache.
         *
         * @param line the line index to "translate"
         */
        void translateLine(uint64_t &line) const {
            for (size_t i = 0; i < toDelete.size() && toDelete[i] <= line; i++) {
                line++;
            }
        }

        /**
         * Write a row to the file.
         * Replaces any other values on the same line.
         *
         * @param row the row to write
         * @param line the line of the row to write
         */
        void writeToFile(const csv_row<string_type, Sep, _escape_generator_> &row, uint64_t line) {
            cache.insert_or_assign(line, row);

            // If the cache size is greater than or equal to
            // the max size, write the cache to the file
            if (getCacheSize() >= maxCached) {
                writeCacheToFile();
            }
        }

        /**
         * Get or create a row.
         * Basically creates a row if it doesn't
         * exist or returns the existing one.
         *
         * @param line the zero-based index of the line to get (translated)
         * @return the created or retrieved row
         */
        csv_row<string_type, Sep, _escape_generator_> &getOrCreateLine(uint64_t line) {
            if (line <= getTranslatedMaxLineIndex()) {
                return getLine(line);
            } else {
                // If line is greater than currentLine,
                // set currentLine to line
                if (line > currentLine) currentLine = line;
                writeToFile(csv_row<string_type, Sep, _escape_generator_>(nullptr), line);

                if (cache.empty()) {
                    cache.insert_or_assign(line, csv_row<string_type, Sep, _escape_generator_>(nullptr));
                }

                return cache.at(line);
            }
        }

        /**
         * Get a line from the csv file.
         * Returns an empty line if not found.
         *
         * @param line the line to find
         * @return the line read from the file
         */
        CSV_NODISCARD csv_row<string_type, Sep, _escape_generator_> getLineFromFile(uint64_t line) const {
            if (line > getTranslatedMaxLineIndex()) {
                throw exceptions::index_out_of_range_error("The requested line is out of range");
            }

            // Check if the line is in the cache
            const const_cache_iterator it = cache.find(line);
            if (it == cache.end()) {
                // Get a stream and navigate to the line
                stream_type in = getStream(std::ios::in);
                gotoLine(in, line);

                // Read the value of the line
                string_type value;
                std::getline(in, value);
                in.close();

                // Return the value
                if (value.empty()) {
                    return csv_row<string_type, Sep, _escape_generator_>(nullptr);
                } else {
                    return csv_row<string_type, Sep, _escape_generator_>::parse(value);
                }
            } else {
                return it->second;
            }
        }

        /**
         * Get a row at an index.
         * Returns an empty row if the row with that index does not exist.
         *
         * @param line the zero-based index of the line to get
         * @return the retrieved row
         */
        csv_row<string_type, Sep, _escape_generator_> &getLine(uint64_t line) {
            if (line > getTranslatedMaxLineIndex()) {
                throw exceptions::index_out_of_range_error("The requested line is out of range");
            }

            // Check if the line is in the cache
            cache_iterator it = cache.find(line);
            if (it == cache.end()) {
                csv_row<string_type, Sep, _escape_generator_> row = getLineFromFile(line);

                // Return the value
                cache.insert_or_assign(line, row);

                return cache.at(line);
            } else {
                // Return the cached value
                return it->second;
            }
        }

        /**
         * Get the row stored in the current line
         *
         * @return the current row
         */
        CSV_NODISCARD const_csv_row<string_type, Sep, _escape_generator_> getCurrentLine() const {
            return getLineFromFile(currentLine);
        }

        /**
         * Get the row stored in the current line
         *
         * @return the current row
         */
        CSV_NODISCARD csv_row<string_type, Sep, _escape_generator_> &getCurrentLine() {
            return getOrCreateLine(currentLine);
        }

        /**
         * Get a stream to the csv file
         *
         * @param openMode the open mode flags
         * @return the created stream
         */
        CSV_NODISCARD stream_type getStream(std::ios_base::openmode openMode) const {
            return stream_type(path, openMode);
        }

        /**
         * Get the zero-based index of the last line in the file
         *
         * @return the last index
         */
        CSV_NODISCARD uint64_t getLastFileLineIndex() const {
            // Get the stream to the file and count the lines
            stream_type in = getStream(std::ios::in);
            uint64_t lines = std::count(std::istreambuf_iterator<T>(in), std::istreambuf_iterator<T>(), '\n');
            in.close();

            return lines;
        }

        /**
         * Get the highest stored zero-based index in the file or cache.
         * Only works for values that were translated.
         *
         * @return the index of the last line in the file or cache before anything was deleted
         */
        CSV_NODISCARD uint64_t getTranslatedMaxLineIndex() const {
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
         * Get the highest stored zero-based index in the file or cache.
         * Subtracts the number of lines to delete.
         *
         * @return the index of the last line in the file or cache
         */
        CSV_NODISCARD uint64_t getMaxLineIndex() const {
            return getTranslatedMaxLineIndex() - toDelete.size();
        }

        /**
         * Get the path to the temporary file
         *
         * @tparam U the type of the path
         * @return the path as U
         */
        template<class U = string_type>
        CSV_NODISCARD U getTmpFile() const {
            string_type out = path;
            if constexpr (util::is_u8_string_v<string_type>) {
                out += ".tmp";
            } else if constexpr (util::is_u16_string_v<string_type>) {
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
            stream_type out(getTmpFile(), std::ios::out | std::ios::app);
            stream_type in = getStream(std::ios::in);

            const size_t maxLength = maxRowLength();

            // Whether there was already a line written to the output file
            bool lineWritten = false;
            // The current line index
            uint64_t i = 0;
            // The content of the current line in the file
            string_type current;
            // Go to the beginning of the file
            in.seekg(std::ios::beg);
            while (std::getline(in, current)) {
                // Skip lines that were marked for deletion
                if (std::find(toDelete.begin(), toDelete.end(), i) != toDelete.end()) {
                    // Increase i and skip this iteration
                    i++;
                    continue;
                }

                // Prepend a new line if there was already a line written to the file
                if (lineWritten) {
                    out << std::endl;
                } else {
                    lineWritten = true;
                }

                // Check if the cache has a value for the current line.
                // IF so, write that to the tmp file instead of the original value.
                // If not, write the original value to the tmp file
                const const_cache_iterator it = cache.find(i);
                if (it == cache.end()) {
                    out << csv_row<string_type, Sep, _escape_generator_>::parse(current).to_string(maxLength);
                } else {
                    out << it->second.to_string(maxLength);
                    cache.erase(it);
                }

                // Increase the line counter
                i++;
            }

            // Close the input stream
            in.close();

            // Write all values that were not already written to the file into the file
            if (!cache.empty()) {
                // Get the highest line number
                const uint64_t max = getTranslatedMaxLineIndex();
                for (; i <= max; i++) {
                    // Skip lines that were marked for deletion
                    if (std::find(toDelete.begin(), toDelete.end(), i) != toDelete.end()) {
                        // This line was marked for deletion, skip
                        continue;
                    }

                    // Prepend a new line if there was already a line written to the file
                    if (lineWritten) {
                        out << std::endl;
                    } else {
                        lineWritten = true;
                    }

                    // Write the line to the file if a value for it exists in the cache
                    const const_cache_iterator it = cache.find(i);
                    if (it != cache.end()) {
                        out << it->second.to_string(maxLength);
                    } else {
                        out << csv_row<string_type, Sep, _escape_generator_>(nullptr).to_string(maxLength);
                    }
                }
            } else {
                // Append new lines at the end, if required
                const uint64_t max = getTranslatedMaxLineIndex();
                for (; i <= max; i++) {
                    // Skip lines that were marked for deletion
                    if (std::find(toDelete.begin(), toDelete.end(), i) != toDelete.end()) {
                        // This line was marked for deletion, skip
                        continue;
                    }

                    // Prepend a new line if there was already a line written to the file
                    if (lineWritten) {
                        out << std::endl;
                        out << csv_row<string_type, Sep, _escape_generator_>(nullptr).to_string(maxLength);
                    } else {
                        lineWritten = true;
                    }
                }
            }

            // Flush and close the output stream
            out.flush();
            out.close();

            // Clear the cache
            cache.clear();
            toDelete.clear();

            // Delete the old csv file and rename the tmp file to it
            std::remove(util::string_as<std::string>(path).c_str());
            std::rename(getTmpFile<std::string>().c_str(), util::string_as<std::string>(path).c_str());

            // Assign currentLine to the last line in the file
            currentLine = getLastFileLineIndex();
        }

        CSV_NODISCARD size_t getCacheSize() const {
            return cache.size() + toDelete.size();
        }

        /**
         * Move a stream to a line in a file.
         * Source: https://stackoverflow.com/a/5207600
         *
         * @param stream the stream to move
         * @param num the line to move to
         */
        static void gotoLine(stream_type &stream, uint64_t num) {
            stream.seekg(std::ios::beg);
            for (size_t i = 0; i < num; i++) {
                stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }

        // The lines to delete on flush
        std::vector<uint64_t> toDelete;

        // The maximum amount of cached values
        size_t maxCached;

        // The row cache
        std::map<uint64_t, csv_row<string_type, Sep, _escape_generator_>> cache;

        // The path to the file
        string_type path;

        // The zero-based index of the current line
        uint64_t currentLine;
    };
} //namespace markusjx

#endif //MARKUSJX_CSV_BASIC_CSV_FILE_HPP

namespace markusjx {
    /**
     * A utf-8 csv object
     */
    using csv = basic_csv<std::string, MARKUSJX_CSV_SEPARATOR, util::escape_sequence_generator<std::string, MARKUSJX_CSV_SEPARATOR>>;

    /**
     * A utf-16 csv object
     */
    using w_csv = basic_csv<std::wstring, MARKUSJX_CSV_SEPARATOR, util::escape_sequence_generator<std::wstring, MARKUSJX_CSV_SEPARATOR>>;

    /**
     * A utf-8 csv file
     */
    using csv_file = basic_csv_file<char, MARKUSJX_CSV_SEPARATOR, util::escape_sequence_generator<util::std_basic_string<char>, MARKUSJX_CSV_SEPARATOR>>;

    /**
     * A utf-16 csv file
     */
    using w_csv_file = basic_csv_file<wchar_t, MARKUSJX_CSV_SEPARATOR, util::escape_sequence_generator<util::std_basic_string<wchar_t>, MARKUSJX_CSV_SEPARATOR>>;
} //namespace markusjx

namespace std {
    template<class T, char Sep, class _gen_>
    inline markusjx::basic_csv<T, Sep, _gen_> &endl(markusjx::basic_csv<T, Sep, _gen_> &csv) {
        return csv.endline();
    }

    template<class T, char Sep, class _gen_>
    inline markusjx::basic_csv_file<T, Sep, _gen_> &endl(markusjx::basic_csv_file<T, Sep, _gen_> &file) {
        return file.endline();
    }
}

/**
 * Operator ""
 *
 * @param str the string to parse
 * @param len the length of the string
 * @return the parsed csv object
 */
inline markusjx::csv operator "" __csv(const char *str, size_t len) {
    return markusjx::csv::parse(std::string(str, len));
}

/**
 * Operator ""
 *
 * @param str the string to parse
 * @param len the length of the string
 * @return the parsed csv object
 */
inline markusjx::w_csv operator "" __csv(const wchar_t *str, size_t len) {
    return markusjx::w_csv::parse(std::wstring(str, len));
}

// Un-define everything
#undef CSV_NODISCARD
#undef CSV_CHECK_T_SUPPORTED
#undef CSV_WINDOWS
#undef CSV_UNIX
#undef CSV_REQUIRES
#undef CSV_ENABLE_IF

#endif //MARKUSJX_CSV_CSV_HPP
