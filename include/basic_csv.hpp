#ifndef MARKUSJX_CSV_BASIC_CSV_HPP
#define MARKUSJX_CSV_BASIC_CSV_HPP

#include <cstring>
#include <vector>
#include <ostream>

#include "definitions.hpp"
#include "util.hpp"
#include "escape_sequence_generator.hpp"
#include "csv_cell.hpp"
#include "csv_row.hpp"
#include "basic_csv_file_def.hpp"

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
         * @return the current iterator
         */
        auto erase(size_t index) {
            return this->rows.erase(this->rows.begin() + index);
        }

        /**
         * Remove a row by its iterator
         *
         * @param iter the iterator to identify the row by
         * @return the new current iterator
         */
        row_iterator erase(const const_row_iterator &iter) {
            return this->rows.erase(iter);
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
        CSV_NODISCARD size_t max_row_length() const {
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
        CSV_NODISCARD uint64_t num_elements() const {
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
            const size_t max = max_row_length();
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
