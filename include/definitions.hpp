#ifndef MARKUSJX_CSV_DEFINITIONS_HPP
#define MARKUSJX_CSV_DEFINITIONS_HPP

#if defined(_MSVC_LANG) || defined(__cplusplus)
#   if (defined(_MSVC_LANG) && _MSVC_LANG > 201703L) || __cplusplus > 201703L // C++20
#       define CSV_REQUIRES(type, ...) template<class = type> requires ::markusjx::util::is_any_of_v<type, __VA_ARGS__>
#   endif
#   if (defined(_MSVC_LANG) && _MSVC_LANG > 201402L) || __cplusplus > 201402L // C++17
#       ifndef CSV_REQUIRES
//          Use SFINAE to disable functions
#           define CSV_REQUIRES(type, ...) template<class _SFINAE_PARAM_ = type, class = \
                            typename std::enable_if_t<::markusjx::util::is_any_of_v<_SFINAE_PARAM_, __VA_ARGS__>, int>>
#       endif //CSV_REQUIRES
//      This looks awful, but it works
#       define CSV_ENABLE_IF(...) class = typename std::enable_if_t<__VA_ARGS__, int>
#       if defined(__APPLE__) && defined(__clang__)
#           define CSV_NODISCARD __attribute__ ((warn_unused_result))
#           warning Compiling on apple with clang may lead to issues. Consider compiling using gcc
#       else
#           define CSV_NODISCARD [[nodiscard]]
#       endif
#   else
#       error Use of C++14 and lower is not (yet) supported. Please use C++17 or C++20 instead
#   endif
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#   define CSV_WINDOWS
#   undef CSV_UNIX
#elif defined(__LINUX__) || defined(__APPLE__) || defined (__CYGWIN__) || defined(__linux__) || defined(__FreeBSD__) || \
        defined(unix) || defined(__unix) || defined(__unix__)
#   define CSV_UNIX
#   undef CSV_WINDOWS
#endif

#ifndef CSV_NODISCARD
#   define CSV_NODISCARD
#endif

#ifndef CSV_REQUIRES
#   define CSV_REQUIRES(type, ...)
#endif //CSV_REQUIRES

// Check if <T> is supported
#define CSV_CHECK_T_SUPPORTED static_assert(::markusjx::util::is_u8_string_v<T> || ::markusjx::util::is_u16_string_v<T>,\
                                            "T must be one of: [std::string, std::wstring]");

#ifndef MARKUSJX_CSV_SEPARATOR
#   define MARKUSJX_CSV_SEPARATOR ';'
#endif //MARKUSJX_CSV_SEPARATOR

#endif //MARKUSJX_CSV_DEFINITIONS_HPP
