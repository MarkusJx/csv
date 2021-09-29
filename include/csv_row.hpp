#ifndef MARKUSJX_CSV_CSV_ROW_HPP
#define MARKUSJX_CSV_CSV_ROW_HPP

#include <cstring>
#include <vector>

#include "definitions.hpp"
#include "escape_sequence_generator.hpp"
#include "csv_cell.hpp"
#include "const_csv_row.hpp"

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
         * @return the new iterator
         */
        auto erase(size_t index) {
            return this->cells.erase(this->cells.begin() + index);
        }

        /**
         * Remove a value from this row
         *
         * @param iter the iterator pointing to the value to remove
         * @return the new iterator
         */
        cell_iterator erase(const const_cell_iterator &iter) {
            return this->cells.erase(iter);
        }

        /**
         * Remove all empty cells at the end of this row
         */
        void strip() {
            while (this->cells.end() > this->cells.begin() && (this->cells.end() - 1)->empty()) {
                this->cells.pop_back();
            }
        }

        /// Default destructor
        ~csv_row() noexcept override = default;
    };
} //namespace markusjx

#endif //MARKUSJX_CSV_CSV_ROW_HPP
