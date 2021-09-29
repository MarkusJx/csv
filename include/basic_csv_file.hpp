#ifndef MARKUSJX_CSV_BASIC_CSV_FILE_HPP
#define MARKUSJX_CSV_BASIC_CSV_FILE_HPP

#include <cstring>
#include <vector>
#include <map>
#include <fstream>

#include "definitions.hpp"
#include "util.hpp"
#include "escape_sequence_generator.hpp"
#include "const_csv_row.hpp"
#include "csv_row.hpp"
#include "basic_csv_file_def.hpp"
#include "basic_csv.hpp"
#include "index_iterator.hpp"

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

        using iterator = index_iterator<basic_csv_file<T, Sep, _escape_generator_>, csv_row<string_type, Sep, _escape_generator_>>;
        using const_iterator = const_index_iterator<basic_csv_file<T, Sep, _escape_generator_>, const_csv_row<string_type, Sep, _escape_generator_>>;

        /**
         * Create a csv file
         *
         * @param path the path to the file
         * @param maxCached the number of cached elements
         */
        explicit basic_csv_file(const string_type &path, size_t maxCached = 100)
                : maxCached(maxCached), cache(), path(path) {
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
            this->push(csv);
            return *this;
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
            // Add a new line if the current line is not empty
            if (!getCurrentLine().empty()) {
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
            // Write the cache to the file if it's full
            if (cache.find(line) == cache.end() && getCacheSize() >= maxCached) {
                writeCacheToFile();
            }

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
         * Get the begin iterator
         *
         * @return the begin iterator
         */
        iterator begin() {
            return iterator(this, 0);
        }

        /**
         * Get the const begin iterator
         *
         * @return the const begin iterator
         */
        const_iterator begin() const {
            return const_iterator(this, 0);
        }

        /**
         * Get the end iterator
         *
         * @return the end iterator
         */
        iterator end() {
            return iterator(this, size());
        }

        /**
         * Get the const end iterator
         *
         * @return the const end iterator
         */
        const_iterator end() const {
            return const_iterator(this, size());
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
            uint64_t res = getMaxLineIndex() + 1;
            if (res == 1 && is_file_empty()) {
                return 0;
            } else {
                return res;
            }
        }

        /**
         * Check if the file is empty.
         * Only checks if the actual file on the
         * disk is empty, not if there is data in the cache.
         *
         * @return true if the file is empty
         */
        CSV_NODISCARD bool is_file_empty() const {
            stream_type ifs = getStream(std::ios::in);
            if (!ifs) {
                throw exceptions::file_operation_error("Could not open the file stream");
            } else {
                return ifs.peek() == std::ifstream::traits_type::eof();
            }
        }

        /**
         * Check if the csv file is empty.
         * Also checks if the cache is empty.
         *
         * @return true if the csv file is empty
         */
        CSV_NODISCARD bool empty() const {
            return size() <= 0;
        }

        /**
         * Remove a csv row at an index
         *
         * @param index the zero-based index of the row to delete
         */
        iterator erase(uint64_t index) {
            if (index > getMaxLineIndex()) {
                throw exceptions::index_out_of_range_error("The requested index is out of range");
            }

            uint64_t index_cpy = index;

            // Translate the index
            translateLine(index);

            // Erase the line from the cache if it is stored in there
            if (const const_cache_iterator it = cache.find(index); it != cache.end()) {
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

            if (index_cpy > getMaxLineIndex()) {
                return this->end();
            } else {
                return this->begin() + index_cpy;
            }
        }

        iterator erase(const const_iterator &iter) {
            return this->erase(iter - this->begin());
        }

        iterator erase(const iterator &iter) {
            return this->erase(iter - this->begin());
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
        CSV_NODISCARD size_t max_row_length() const {
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

            const size_t maxLength = max_row_length();

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
                if (const const_cache_iterator it = cache.find(i); it == cache.end()) {
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
