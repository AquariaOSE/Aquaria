// Minimal stdint.h essentials, loosely based on Paul Hsieh's pstdint.h
// See http://www.azillionmonkeys.com/qed/pstdint.h for reference.
// This adds only the xint*_t types, xINT*_C() macros, and xINT*_MAX defines for older versions of visual studio.
// Might want to use the full pstdint.h instead if needed to support older compilers.

#ifndef _PSTDINT_H_INCLUDED
#define _PSTDINT_H_INCLUDED

#include <limits.h>

#if defined(_MSC_VER) && _MSC_VER < 1600
#include "SDL_version.h"
#if SDL_VERSION_ATLEAST(2, 0, 0) // AQUARIA HACK: Included SDL 1.2 includes define some of these, SDL does not. Avoid conflicts.
    typedef signed __int8     int8_t;
    typedef unsigned __int8   uint8_t;
    typedef signed __int16    int16_t;
    typedef unsigned __int16  uint16_t;
    typedef signed __int32    int32_t;
    typedef unsigned __int32  uint32_t;
#else
#include "SDL_config.h"
#endif
    typedef signed __int64    int64_t;
    typedef unsigned __int64  uint64_t;
#   ifdef __cplusplus
        namespace std
        {
            typedef ::int8_t int8_t;
            typedef ::int16_t int16_t;
            typedef ::int32_t int32_t;
            typedef ::int64_t int64_t;
            typedef ::uint8_t uint8_t;
            typedef ::uint16_t uint16_t;
            typedef ::uint32_t uint32_t;
            typedef ::uint64_t uint64_t;
        }
#   endif
#   define INT8_C(v) ((int8_t)v)
#   define UINT8_C(v) ((uint8_t)v)
#   define INT16_C(v) ((int16_t)v)
#   define UINT16_C(v) ((uint16_t)v)
#   define INT32_C(v) ((int32_t)v)
#   define UINT32_C(v) ((uint32_t)v)
#   define UINT64_C(v) (v ## UI64)
#   define INT64_C(v) (v ## I64)
#else
#   ifdef __cplusplus
#       include <cstdint>
#   endif
#   include <stdint.h>
#endif

# if !defined (UINT64_MAX)
# define UINT64_MAX UINT64_C(18446744073709551615)
# endif
# if !defined (INT64_MAX)
# define INT64_MAX INT64_C(9223372036854775807)
# endif
# if !defined (UINT32_MAX)
# define UINT32_MAX UINT32_C(4294967295UL)
# endif
# if !defined (INT32_MAX)
# define INT32_MAX INT32_C(2147483647L)
# endif
#ifndef UINT16_MAX
# define UINT16_MAX 0xffff
#endif
#ifndef INT16_MAX
# define INT16_MAX 0x7fff
#endif
#ifndef UINT8_MAX
# define UINT8_MAX 0xff
#endif
#ifndef INT8_MAX
# define INT8_MAX 0x7f
#endif

#ifndef INT8_MIN
# define INT8_MIN INT8_C(0x80)
#endif
#ifndef INT16_MIN
# define INT16_MIN INT16_C(0x8000)
#endif
#ifndef INT32_MIN
# define INT32_MIN INT32_C(0x80000000)
#endif
#ifndef INT64_MIN
# define INT64_MIN INT64_C(0x8000000000000000)
#endif

#endif

