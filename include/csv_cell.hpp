#ifndef MARKUSJX_CSV_CSV_CELL_HPP
#define MARKUSJX_CSV_CSV_CELL_HPP

#include <cstring>
#include <string>
#include <regex>

#include "definitions.hpp"
#include "util.hpp"
#include "escape_sequence_generator.hpp"
#include "exceptions.hpp"

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
