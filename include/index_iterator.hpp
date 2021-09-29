#ifndef MARKUSJX_CSV_INDEX_ITERATOR_HPP
#define MARKUSJX_CSV_INDEX_ITERATOR_HPP

#include <iterator>

#include "definitions.hpp"

namespace markusjx {
    /**
     * A constant iterator to iterator over indices.
     * Just returns copies of the value type.
     * Requires T to have a function at().
     *
     * @tparam T the type to iterate over
     * @tparam U the value type
     */
    template<class T, class U>
    class const_index_iterator;

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
        using difference_type = ptrdiff_t;
        using value_type = U;
        using index_type = uint64_t;

        /**
         * Create an index iterator
         *
         * @param data the data to iterate over
         * @param pos the position to start at
         */
        index_iterator(T *data, index_type pos) : data(data), pos(pos) {}

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
         * Prefix decrement
         *
         * @return this
         */
        index_iterator &operator--() {
            pos--;
            return *this;
        }

        /**
         * Postfix decrement
         *
         * @return this before it was decreased
         */
        index_iterator operator--(int) {
            index_iterator tmp = *this;
            --pos;
            return tmp;
        }

        CSV_NODISCARD index_type operator-(const index_iterator &it) const {
            return pos - it.pos;
        }

        CSV_NODISCARD inline index_type operator-(const const_index_iterator<T, U> &it) const;

        template<class N>
        CSV_NODISCARD index_iterator operator+(N val) const {
            index_iterator tmp = *this;
            tmp.pos += static_cast<index_type>(val);
            return tmp;
        }

        template<class N>
        CSV_NODISCARD index_iterator operator-(N val) const {
            index_iterator tmp = *this;
            tmp.pos -= static_cast<index_type>(val);
            return tmp;
        }

        template<class N>
        CSV_NODISCARD index_iterator operator*(N val) const {
            index_iterator tmp = *this;
            tmp.pos *= static_cast<index_type>(val);
            return tmp;
        }

        template<class N>
        CSV_NODISCARD index_iterator operator/(N val) const {
            index_iterator tmp = *this;
            tmp.pos /= static_cast<index_type>(val);
            return tmp;
        }

        template<class N>
        index_iterator &operator+=(N val) {
            pos += static_cast<index_type>(val);
            return *this;
        }

        template<class N>
        index_iterator &operator-=(N val) {
            pos -= static_cast<index_type>(val);
            return *this;
        }

        template<class N>
        index_iterator &operator*=(N val) {
            pos *= static_cast<index_type>(val);
            return *this;
        }

        template<class N>
        index_iterator &operator/=(N val) {
            pos /= static_cast<index_type>(val);
            return *this;
        }

        CSV_NODISCARD index_type position() const {
            return pos;
        }

        CSV_NODISCARD T *get_data() const {
            return data;
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
        index_type pos;
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
        using difference_type = ptrdiff_t;
        using value_type = U;
        using index_type = uint64_t;

        /**
         * Create a constant index iterator
         *
         * @param data the data to iterate over
         * @param pos the position to start at
         */
        const_index_iterator(const T *data, index_type pos) : data(data), pos(pos) {}

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
            const_index_iterator tmp = *this;
            ++pos;
            return tmp;
        }

        /**
         * Prefix decrement
         *
         * @return this
         */
        const_index_iterator &operator--() {
            pos--;
            return *this;
        }

        /**
         * Postfix decrement
         *
         * @return this before it was decreased
         */
        const_index_iterator operator--(int) {
            const_index_iterator tmp = *this;
            --pos;
            return tmp;
        }

        CSV_NODISCARD index_type operator-(const const_index_iterator &it) const {
            return pos - it.pos;
        }

        CSV_NODISCARD index_type operator-(const index_iterator<T, U> &it) const {
            return pos - it.position();
        }

        template<class N>
        CSV_NODISCARD const_index_iterator operator+(N val) const {
            index_iterator tmp = *this;
            tmp.pos += static_cast<index_type>(val);
            return tmp;
        }

        template<class N>
        CSV_NODISCARD const_index_iterator operator-(N val) const {
            index_iterator tmp = *this;
            tmp.pos -= static_cast<index_type>(val);
            return tmp;
        }

        template<class N>
        CSV_NODISCARD const_index_iterator operator*(N val) const {
            index_iterator tmp = *this;
            tmp.pos *= static_cast<index_type>(val);
            return tmp;
        }

        template<class N>
        CSV_NODISCARD const_index_iterator operator/(N val) const {
            index_iterator tmp = *this;
            tmp.pos /= static_cast<index_type>(val);
            return tmp;
        }

        template<class N>
        const_index_iterator &operator+=(N val) {
            pos += static_cast<index_type>(val);
            return *this;
        }

        template<class N>
        const_index_iterator &operator-=(N val) {
            pos -= static_cast<index_type>(val);
            return *this;
        }

        template<class N>
        const_index_iterator &operator*=(N val) {
            pos *= static_cast<index_type>(val);
            return *this;
        }

        template<class N>
        const_index_iterator &operator/=(N val) {
            pos /= static_cast<index_type>(val);
            return *this;
        }

        CSV_NODISCARD index_type position() const {
            return pos;
        }

        CSV_NODISCARD T *get_data() const {
            return data;
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

    template<class T, class U>
    CSV_NODISCARD inline typename index_iterator<T, U>::index_type
    index_iterator<T, U>::operator-(const const_index_iterator<T, U> &it) const {
        return pos - it.position();
    }
} //namespace markusjx

#endif //MARKUSJX_CSV_INDEX_ITERATOR_HPP
