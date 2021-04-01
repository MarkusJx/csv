#ifndef MARKUSJX_CSV_BASIC_CSV_FILE_DEF_HPP
#define MARKUSJX_CSV_BASIC_CSV_FILE_DEF_HPP

#include "escape_sequence_generator.hpp"

namespace markusjx {
    /**
     * A csv file
     *
     * @tparam T the char type of the file
     * @tparam Sep the separator to use
     * @tparam _escape_generator_ the escape sequence generator to use
     */
    template<class T, char Sep = ';', class _escape_generator_ = util::escape_sequence_generator<util::std_basic_string<T>, Sep>>
    class basic_csv_file;
} //namespace markusjx

#endif //MARKUSJX_CSV_BASIC_CSV_FILE_DEF_HPP
