#ifndef MARKUSJX_CSV_EXCEPTIONS_HPP
#define MARKUSJX_CSV_EXCEPTIONS_HPP

#include <stdexcept>

#include "definitions.hpp"

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
