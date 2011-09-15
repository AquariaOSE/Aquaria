#ifndef LVPA_INTERNAL_H
#define LVPA_INTERNAL_H


#ifdef _DEBUG
#  define DBG if(1)
#  define DEBUG(x) x;
#  define logdebug(...) { printf(__VA_ARGS__); putchar('\n'); }
#  define logerror(...) { fputs("ERROR: ",stdout); printf(__VA_ARGS__); putchar('\n'); }
#else
#  define DBG if(0)
#  define DEBUG(x)
#  define logdebug(...)
#  define logerror(...)
#endif


//////////////////////////////////////
// Platform defines
//////////////////////////////////////

#define PLATFORM_WIN32 0
#define PLATFORM_UNIX  1
#define PLATFORM_APPLE 2
#define PLATFORM_INTEL 3

#if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )
#  define PLATFORM PLATFORM_WIN32
#elif defined( __APPLE_CC__ )
#  define PLATFORM PLATFORM_APPLE
#elif defined( __INTEL_COMPILER )
#  define PLATFORM PLATFORM_INTEL
#else
#  define PLATFORM PLATFORM_UNIX
#endif

#define COMPILER_MICROSOFT 0
#define COMPILER_GNU       1
#define COMPILER_BORLAND   2
#define COMPILER_INTEL     3

#ifdef _MSC_VER
#  define COMPILER COMPILER_MICROSOFT
#elif defined( __BORLANDC__ )
#  define COMPILER COMPILER_BORLAND
#elif defined( __INTEL_COMPILER )
#  define COMPILER COMPILER_INTEL
#elif defined( __GNUC__ )
#  define COMPILER COMPILER_GNU
#else
#  pragma error "FATAL ERROR: Unknown compiler."
#endif

// stupid warnings
#if COMPILER == COMPILER_MICROSOFT
#  define _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_DEPRECATE
#  pragma warning(disable: 4996)
#endif

////////////////////////////////////
// Compiler defines
////////////////////////////////////

#if COMPILER == COMPILER_MICROSOFT
    #define I64FMT "%016I64X"
    #define I64FMTD "%I64u"
    #define I64LIT(x) (x ## i64)
    #define UI64LIT(x) (x ## ui64)
    #define snprintf _snprintf
#else
    #define stricmp strcasecmp
    #define strnicmp strncasecmp
    #define I64FMT "%016llX"
    #define I64FMTD "%llu"
    #define I64LIT(x) (x ## LL)
    #define UI64LIT(x) (x ## ULL)
#endif

#ifndef _LP64
#   if defined (_M_IA64) || defined (__ia64__) || defined (_M_AMD64) || defined (__amd64) || defined(_M_X64)
#      define _LP64 1
#   endif
#endif

#ifdef _LP64 // to be set for 64 bit compile
#   define PTRFMT "0x"I64FMT
#   define SYSTEM_BITS 64
#else
#   define PTRFMT "0x%X"
#   define SYSTEM_BITS 32
#endif

#ifndef SIGQUIT
#define SIGQUIT 3
#endif

#if COMPILER == COMPILER_MICROSOFT
#  if _MSC_VER >= 1600
#     define COMPILER_NAME "VC100+"
#  elif _MSC_VER >= 1500
#    define COMPILER_NAME "VC90"
#  elif _MSC_VER >= 1400
#    define COMPILER_NAME "VC80"
#  elif _MSC_VER >= 1310
#    define COMPILER_NAME "VC71"
#  endif
#  define COMPILER_VERSION _MSC_VER
#  define COMPILER_VERSION_OUT "%u"
#elif COMPILER == COMPILER_GNU
#  define COMPILER_NAME "GCC"
#  ifndef __GNUC_PATCHLEVEL__
#    define __GNUC_PATCHLEVEL__ 0
#  endif
#  define COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#  define COMPILER_VERSION_OUT "%u"
// TODO: add more compilers here when necessary
#else
#  define COMPILER_NAME "unknown"
#  define COMPILER_VERSION "unk"
#  define COMPILER_VERSION_OUT "%s"
#endif

#if PLATFORM == PLATFORM_UNIX
# define PLATFORM_NAME "Unix"
#elif PLATFORM == PLATFORM_WIN32
# define PLATFORM_NAME "Win32"
#elif PLATFORM == PLATFORM_APPLE
# define PLATFORM_NAME "Apple"
// TODO: add more platforms here when necessary
#else
# define PLATFORM_NAME "unknown"
#endif

#if COMPILER == COMPILER_GNU
#  define ATTR_NORETURN __attribute__((noreturn))
#  define ATTR_PRINTF(F,V) __attribute__ ((format (printf, F, V)))
#else //COMPILER != COMPILER_GNU
#  define ATTR_NORETURN
#  define ATTR_PRINTF(F,V)
#endif //COMPILER == COMPILER_GNU


// taken from ACE
// have seen on some systems that both defines exist, so if that is is the case, rely on this detection here
#if (!defined(LITTLE_ENDIAN) && !defined(BIG_ENDIAN)) || (defined(LITTLE_ENDIAN) && defined(BIG_ENDIAN))
#  if defined (i386) || defined (__i386__) || defined (_M_IX86) || \
      defined (vax) || defined (__alpha) || defined (__LITTLE_ENDIAN__) || \
      defined (ARM) || defined (_M_IA64) || defined (__ia64__) || \
      defined (_M_AMD64) || defined (__amd64)
         // We know these are little endian.
#        undef  LITTLE_ENDIAN
#        undef  BIG_ENDIAN
#        define LITTLE_ENDIAN 1
#        define IS_LITTLE_ENDIAN 1
#        define IS_BIG_ENDIAN 0
#  else
         // Otherwise, we assume big endian.
#        undef  LITTLE_ENDIAN
#        undef  BIG_ENDIAN
#        define BIG_ENDIAN 1
#        define IS_LITTLE_ENDIAN 0
#        define IS_BIG_ENDIAN 1
#  endif
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#define ASSERT(what) { if (!(what)) { fprintf( stderr, "\n%s:%i in %s ASSERTION FAILED:\n  %s\n", __FILE__, __LINE__,__FUNCTION__,  #what); assert( #what &&0 ); } }



#include "LVPACommon.h"

LVPA_NAMESPACE_START


template <typename T> class AutoPtrVector
{
public:
    inline AutoPtrVector(uint32 prealloc) :v(prealloc)
    {
        for(uint32 i = 0; i < prealloc; ++i)
            v[i] = NULL;
    }
    inline ~AutoPtrVector()
    {
        for(uint32 i = 0; i < v.size(); ++i)
            if(v[i])
                delete v[i];
    }
    std::vector<T*> v;
};

LVPA_NAMESPACE_END

#endif
