#ifndef CSVGENERATOR_CSV_HPP
#define CSVGENERATOR_CSV_HPP

#include <vector>
#include <sstream>
#include <string>
#include <functional>

#if defined(_MSVC_LANG) || defined(__cplusplus)
#   if (defined(_MSVC_LANG) && _MSVC_LANG > 201402L) || __cplusplus > 201402L // C++17
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

#ifndef CSV_NODISCARD
#   define CSV_NODISCARD
#endif

#define CSV_CHECK_T_SUPPORTED static_assert(util::is_any_of_v<T, std::string, std::wstring>, "T must be one of: [std::string, std::wstring]");

namespace markusjx {
    namespace util {
        template<class T, class... Types>
        inline constexpr bool is_any_of_v = std::disjunction_v<std::is_same<T, Types>...>;

        template<class T>
        inline std::vector<T> splitString(const T &str, char delimiter) {
            std::vector<T> tokens;
            size_t prev = 0, pos;
            do {
                pos = str.find(delimiter, prev);
                if (pos == T::npos) pos = str.length();
                T token = str.substr(prev, pos - prev);
                if (!token.empty()) tokens.push_back(token);
                prev = pos + 1;
            } while (pos < str.length() && prev < str.length());
            return tokens;
        }

        template<class T, class S>
        inline T stringToNumberAlt(const S &str, T(*conversionFunc)(const S &, size_t *)) {
            size_t idx = 0;
            T res = conversionFunc(str, &idx);

            if (idx != 0) {
                throw std::runtime_error("Could not fully convert the value");
            } else {
                return res;
            }
        }

        template<class T, class S>
        inline T stringToNumber(const S &str, T(*conversionFunc)(const S &, size_t *, int)) {
            size_t idx = 0;
            T res = conversionFunc(str, &idx, 10);

            if (idx != 0) {
                throw std::runtime_error("Could not fully convert the value");
            } else {
                return res;
            }
        }

        inline std::string wstring_to_string(const std::wstring &in) {
            std::string out(in.size() + 1, ' ');
            size_t outSize;

            errno_t err = wcstombs_s(&outSize, out.data(), out.size(), in.c_str(), in.size());
            if (err) {
                throw std::runtime_error("Could not create the string");
            }

            out.resize(outSize);
            return out;
        }

        inline std::wstring string_to_wstring(const std::string &in) {
            std::wstring out(in.size() + 1, L' ');

            size_t outSize;
            errno_t err = mbstowcs_s(&outSize, (wchar_t *) out.data(), out.size(), in.c_str(), in.size());
            if (err) {
                throw std::runtime_error("Could not create the string");
            }

            out.resize(outSize);
            return out;
        }
    }

    template<class T>
    class csvrowcolumn {
    public:
        CSV_CHECK_T_SUPPORTED

        template<class U>
        static csvrowcolumn<U> parse(const U &value) {
            csvrowcolumn<U> col(nullptr);
            col.value = value;

            return col;
        }

        explicit csvrowcolumn(std::nullptr_t) : value() {}

        csvrowcolumn<T> &operator=(const std::string &val) {
            value = '"' + val + '"';
            return *this;
        }

        csvrowcolumn<T> &operator=(const char *val) {
            value = '"' + T(val) + '"';
            return *this;
        }

        csvrowcolumn<T> &operator=(const wchar_t *val) {
            value = L'"' + T(val) + L'"';
            return *this;
        }

        csvrowcolumn<T> &operator=(bool val) {
            if constexpr (std::is_same_v<T, std::string>) {
                value = '"' + std::string(val ? "true" : "false") + '"';
            } else if constexpr (std::is_same_v<T, std::wstring>) {
                value = L'"' + std::wstring(val ? L"true" : L"false") + L'"';
            }

            return *this;
        }

        template<class U>
        csvrowcolumn &operator=(const U &val) {
            if constexpr (std::is_same_v<T, std::string>) {
                value = std::to_string(val);
            } else if constexpr (std::is_same_v<T, std::wstring>) {
                value = std::to_wstring(val);
            }

            return *this;
        }

        CSV_NODISCARD T rawValue() const {
            return value;
        }

        CSV_NODISCARD operator T() const {
            if (value.starts_with('"')) {
                return value.substr(1, value.size() - 2);
            } else {
                throw std::runtime_error("The stored value is not a string");
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
            T val = this->operator T();
            if constexpr (std::is_same_v<T, std::string>) {
                return op_bool_impl(val, "true", "false");
            } else if constexpr (std::is_same_v<T, std::wstring>) {
                return op_bool_impl(val, L"true", L"false");
            }
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

        template<class U>
        static csvrow<U> parse(const U &value, const char separator) {
            csvrow row(nullptr);
            for (const U &col : util::splitString(value, separator)) {
                row << csvrowcolumn<U>::parse(col);
            }

            return row;
        }

        explicit csvrow(std::nullptr_t) : columns() {}

        csvrowcolumn<T> &operator[](size_t columnIndex) {
            if (columnIndex >= columns.size()) {
                for (size_t i = columns.size(); i <= columnIndex; i++) {
                    columns.emplace_back(nullptr);
                }
            }

            return this->columns[columnIndex];
        }

        csvrow &operator<<(const csvrowcolumn<T> &col) {
            columns.push_back(col);
            return *this;
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

    private:
        std::vector<csvrowcolumn<T>> columns;
    };

    template<class T>
    class basic_csv {
    public:
        CSV_CHECK_T_SUPPORTED

        template<class U>
        static basic_csv<U> parse(const U &value, const char separator = ';') {
            basic_csv<U> csv;
            for (const U &row : util::splitString(value, '\n')) {
                csv << csvrow<U>::parse(row, separator);
            }

            return csv;
        }

        explicit basic_csv(const char separator = ';') : rows(), separator(separator) {}

        csvrow<T> &operator[](size_t rowIndex) {
            if (rowIndex >= this->rows.size()) {
                for (size_t i = rows.size(); i <= rowIndex; i++) {
                    rows.emplace_back(nullptr);
                }
            }

            return rows[rowIndex];
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

        basic_csv<T> &operator<<(basic_csv<T> &(*val)(basic_csv<T> &)) {
            return val(*this);
        }

        CSV_NODISCARD T to_string() const {
            if constexpr (std::is_same_v<T, std::string>) {
                return to_string_impl<std::stringstream>();
            } else if constexpr (std::is_same_v<T, std::wstring>) {
                return to_string_impl<std::wstringstream>();
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

        static basic_csv<T> &endl(basic_csv<T> &c) {
            c.rows.emplace_back(nullptr);
            return c;
        }

        template<class U>
        friend std::ostream &operator<<(std::ostream &ostream, const basic_csv<U> &csv) {
            if constexpr (std::is_same_v<T, std::string>) {
                ostream << csv.to_string();
            } else if constexpr (std::is_same_v<T, std::wstring>) {
                ostream << util::wstring_to_string(csv.to_string());
            }

            return ostream;
        }

        template<class U>
        friend std::wostream &operator<<(std::wostream &ostream, const basic_csv<U> &csv) {
            if constexpr (std::is_same_v<T, std::wstring>) {
                ostream << csv.to_string();
            } else if constexpr (std::is_same_v<T, std::string>) {
                ostream << util::string_to_wstring(csv.to_string());
            }

            return ostream;
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

        std::vector<csvrow<T>> rows;
        char separator;
    };

    using csv = basic_csv<std::string>;
}

#undef CSV_NODISCARD
#undef CSV_CHECK_T_SUPPORTED

#endif //CSVGENERATOR_CSV_HPP
