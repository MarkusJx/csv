#ifndef MARKUSJX_CSV_INDEX_ITERATOR_HPP
#define MARKUSJX_CSV_INDEX_ITERATOR_HPP

#include <iterator>

#include "definitions.hpp"

namespace markusjx {
    /**
     * An iterator to iterator over indices.
     * Requires T to have a function at().
     *
     * @tparam T the type to iterate over
     * @tparam U the value type
     */
    template<class T, class U>
    class index_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = int64_t;
        using value_type = U;

        /**
         * Create an index iterator
         *
         * @param data the data to iterate over
         * @param pos the position to start at
         */
        index_iterator(T *data, uint64_t pos) : data(data), pos(pos) {}

        /**
         * Get a reference to the value
         *
         * @return the value
         */
        CSV_NODISCARD value_type &operator*() const {
            return data->at(pos);
        }

        /**
         * Get the value pointer
         *
         * @return the value
         */
        value_type *operator->() {
            return &data->at(pos);
        }

        /**
         * Prefix increment
         *
         * @return this
         */
        index_iterator &operator++() {
            pos++;
            return *this;
        }

        /**
         * Postfix increment
         *
         * @return this before it was increased
         */
        index_iterator operator++(int) {
            index_iterator tmp = *this;
            ++pos;
            return tmp;
        }

        /**
         * Check if this is equal to another index_iterator
         *
         * @param other the iterator to compare to
         * @return true if this is equal to other
         */
        CSV_NODISCARD bool operator==(const index_iterator &other) const {
            return this->pos == other.pos && this->data == other.data;
        };

        /**
         * Check if this is not equal to another index_iterator
         *
         * @param other the iterator to compare to
         * @return true if this is not equal to other
         */
        CSV_NODISCARD bool operator!=(const index_iterator &other) const {
            return this->pos != other.pos || this->data != other.data;
        };

    private:
        // A pointer to the data
        T *data;
        // The current position
        uint64_t pos;
    };

    /**
     * A constant iterator to iterator over indices.
     * Just returns copies of the value type.
     * Requires T to have a function at().
     *
     * @tparam T the type to iterate over
     * @tparam U the value type
     */
    template<class T, class U>
    class const_index_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = int64_t;
        using value_type = U;

        /**
         * Create a constant index iterator
         *
         * @param data the data to iterate over
         * @param pos the position to start at
         */
        const_index_iterator(const T *data, uint64_t pos) : data(data), pos(pos) {}

        /**
         * Get a copy of the value
         *
         * @return the value
         */
        CSV_NODISCARD value_type operator*() const {
            return data->at(pos);
        }

        /**
         * Prefix increment
         *
         * @return this
         */
        const_index_iterator &operator++() {
            pos++;
            return *this;
        }

        /**
         * Postfix increment
         *
         * @return this before it was increased
         */
        const_index_iterator operator++(int) {
            index_iterator tmp = *this;
            ++pos;
            return tmp;
        }

        /**
         * Check if this is equal to another const_index_iterator
         *
         * @param other the iterator to compare to
         * @return true if this is equal to other
         */
        CSV_NODISCARD bool operator==(const const_index_iterator &other) const {
            return this->pos == other.pos && this->data == other.data;
        };

        /**
         * Check if this is not equal to another const_index_iterator
         *
         * @param other the iterator to compare to
         * @return true if this is not equal to other
         */
        CSV_NODISCARD bool operator!=(const const_index_iterator &other) const {
            return this->pos != other.pos || this->data != other.data;
        };

    private:
        // A pointer to the data
        const T *data;
        // The current position
        uint64_t pos;
    };
} //namespace markusjx

#endif //MARKUSJX_CSV_INDEX_ITERATOR_HPP
