#ifndef MARKUSJX_CSV_ESCAPE_SEQUENCE_GENERATOR_HPP
#define MARKUSJX_CSV_ESCAPE_SEQUENCE_GENERATOR_HPP

#include <cstring>
#include <vector>

#include "definitions.hpp"
#include "exceptions.hpp"
#include "util.hpp"

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
