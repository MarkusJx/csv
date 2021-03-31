#ifndef MARKUSJX_CSV_CSV_HPP
#define MARKUSJX_CSV_CSV_HPP

#include <cstring>

#include "definitions.hpp"
#include "escape_sequence_generator.hpp"
#include "basic_csv.hpp"
#include "basic_csv_file.hpp"

namespace markusjx {
    /**
     * A utf-8 csv object
     */
    using csv = basic_csv<std::string, MARKUSJX_CSV_SEPARATOR, util::escape_sequence_generator<std::string, MARKUSJX_CSV_SEPARATOR>>;

    /**
     * A utf-16 csv object
     */
    using w_csv = basic_csv<std::wstring, MARKUSJX_CSV_SEPARATOR, util::escape_sequence_generator<std::wstring, MARKUSJX_CSV_SEPARATOR>>;

    /**
     * A utf-8 csv file
     */
    using csv_file = basic_csv_file<char, MARKUSJX_CSV_SEPARATOR, util::escape_sequence_generator<util::std_basic_string<char>, MARKUSJX_CSV_SEPARATOR>>;

    /**
     * A utf-16 csv file
     */
    using w_csv_file = basic_csv_file<wchar_t, MARKUSJX_CSV_SEPARATOR, util::escape_sequence_generator<util::std_basic_string<wchar_t>, MARKUSJX_CSV_SEPARATOR>>;
} //namespace markusjx

namespace std {
    template<class T, char Sep, class _gen_>
    inline markusjx::basic_csv<T, Sep, _gen_> &endl(markusjx::basic_csv<T, Sep, _gen_> &csv) {
        return csv.endline();
    }

    template<class T, char Sep, class _gen_>
    inline markusjx::basic_csv_file<T, Sep, _gen_> &endl(markusjx::basic_csv_file<T, Sep, _gen_> &file) {
        return file.endline();
    }
}

/**
 * Operator ""
 *
 * @param str the string to parse
 * @param len the length of the string
 * @return the parsed csv object
 */
inline markusjx::csv operator "" __csv(const char *str, size_t len) {
    return markusjx::csv::parse(std::string(str, len));
}

/**
 * Operator ""
 *
 * @param str the string to parse
 * @param len the length of the string
 * @return the parsed csv object
 */
inline markusjx::w_csv operator "" __csv(const wchar_t *str, size_t len) {
    return markusjx::w_csv::parse(std::wstring(str, len));
}

// Un-define everything
#undef CSV_NODISCARD
#undef CSV_CHECK_T_SUPPORTED
#undef CSV_WINDOWS
#undef CSV_UNIX
#undef CSV_REQUIRES
#undef CSV_ENABLE_IF

#endif //MARKUSJX_CSV_CSV_HPP
