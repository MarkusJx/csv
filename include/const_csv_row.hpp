#ifndef MARKUSJX_CSV_CONST_CSV_ROW_HPP
#define MARKUSJX_CSV_CONST_CSV_ROW_HPP

#include <cstring>
#include <vector>

#include "definitions.hpp"
#include "util.hpp"
#include "escape_sequence_generator.hpp"
#include "csv_cell.hpp"

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

        /// Operator << for stream operations
        friend std::ostream &operator<<(std::ostream &stream, const const_csv_row &row) {
            stream << row.to_string();
            return stream;
        }

        /// Default destructor
        virtual ~const_csv_row() noexcept = default;

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
