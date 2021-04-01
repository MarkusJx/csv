#ifndef MARKUSJX_CSV_UTIL_HPP
#define MARKUSJX_CSV_UTIL_HPP

#include <cstdlib>
#include <cstring>
#include <type_traits>

#include "definitions.hpp"
#include "exceptions.hpp"

namespace markusjx::util {
    /**
     * Check if T is any of Types
     *
     * @tparam T the type to check
     * @tparam Types the types to check against
     */
    template<class T, class... Types>
    inline constexpr bool is_any_of_v = std::disjunction_v<std::is_same<T, Types>...>;

    /**
     * Check if T is a utf-8 string
     *
     * @tparam T the type to check
     */
    template<class T>
    inline constexpr bool is_u8_string_v = is_any_of_v<T, std::string>;

    /**
     * Check if T is a utf-16 string
     *
     * @tparam T the type to check
     */
    template<class T>
    inline constexpr bool is_u16_string_v = is_any_of_v<T, std::wstring>;

    /**
     * An alias for std::basic_string
     *
     * @tparam T the type of the string
     */
    template<class T>
    using std_basic_string = std::basic_string<T, std::char_traits<T>, std::allocator<T>>;

    /**
     * Convert a string to a number. Alternative version
     *
     * @tparam T the number type
     * @tparam S the string type
     * @param str the string to convert
     * @param conversionFunc the conversion function
     * @return the converted number
     */
    template<class T, class S>
    inline T stringToNumberAlt(const S &str, T(*conversionFunc)(const S &, size_t *)) {
        size_t idx = 0;
        T res = conversionFunc(str, &idx);

        if (idx != 0 && idx != str.size()) {
            throw exceptions::conversion_error("Could not fully convert the value");
        } else {
            return res;
        }
    }

    /**
     * Convert a string to a number
     *
     * @tparam T the number type
     * @tparam S the string type
     * @param str the string to convert
     * @param conversionFunc the conversion function
     * @return the converted number
     */
    template<class T, class S>
    inline T stringToNumber(const S &str, T(*conversionFunc)(const S &, size_t *, int)) {
        size_t idx = 0;
        T res = conversionFunc(str, &idx, 10);

        if (idx == 0) {
            throw exceptions::conversion_error("Could not convert the value");
        } else {
            return res;
        }
    }

    /**
     * Convert a std::wstring to a std::string
     *
     * @param in the wide string to convert
     * @return the converted string
     */
    inline std::string wstring_to_string(const std::wstring &in) {
        // Create the result string
        std::string out(in.size() + 1, ' ');

#ifdef CSV_WINDOWS
        size_t outSize;

        errno_t err = wcstombs_s(&outSize, out.data(), out.size(), in.c_str(), in.size());
        if (err) {
            throw exceptions::conversion_error("Could not convert the string");
        }
#elif defined(CSV_UNIX)
        size_t written = wcstombs(out.data(), in.c_str(), in.size());
        if (written == static_cast<size_t>(-1)) {
            throw exceptions::conversion_error("Could not convert the string");
        }
#endif // WINODWS OR UNIX

        out.resize(in.size());
        return out;
    }

    /**
     * Convert a std::string to a std::wstring
     *
     * @param in the string to convert
     * @return the converted wide string
     */
    inline std::wstring string_to_wstring(const std::string &in) {
        std::wstring out(in.size() + 1, L' ');

#ifdef CSV_WINDOWS
        size_t outSize;
        errno_t err = mbstowcs_s(&outSize, (wchar_t *) out.data(), out.size(), in.c_str(), in.size());
        if (err) {
            throw exceptions::conversion_error("Could not create the string");
        }
#elif defined(CSV_UNIX)
        size_t written = mbstowcs(out.data(), in.c_str(), in.size());
        if (written == static_cast<size_t>(-1)) {
            throw exceptions::conversion_error("Could not convert the string");
        }
#endif // WINDOWS OR UNIX

        out.resize(in.size());
        return out;
    }

    /**
     * Get a string as another string type
     *
     * @tparam T the string type to convert to
     * @tparam U the type of the string to convert
     * @param str the string to convert
     * @return the converted string
     */
    template<class T, class U, CSV_ENABLE_IF(
            (is_u8_string_v<T> || is_u16_string_v<T>) && (is_u8_string_v<U> || is_u16_string_v<U>)) >
    inline T string_as(const U &str) {
        static_assert(is_u8_string_v<T> || is_u16_string_v<T>);
        static_assert(is_u8_string_v<U> || is_u16_string_v<U>);

        if constexpr ((is_u8_string_v<T> && is_u8_string_v<U>) || (is_u16_string_v<T> && is_u16_string_v<U>)) {
            return str;
        } else if constexpr (is_u8_string_v<T> && is_u16_string_v<U>) {
            return wstring_to_string(str);
        } else if constexpr (is_u16_string_v<T> && is_u8_string_v<U>) {
            return string_to_wstring(str);
        }
    }
} //namespace markusjx::util

#endif //MARKUSJX_CSV_UTIL_HPP
