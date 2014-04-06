// VFSInternal.h - misc things that are not required to be visible outside of the library.
// For conditions of distribution and use, see copyright notice in VFS.h

// !! this file is supposed to be included ONLY from VFS*.cpp files.

#ifndef VFS_INTERNAL_H
#define VFS_INTERNAL_H

// checks to enforce correct including
#ifdef TTVFS_VFS_H
#error Oops, TTVFS_VFS_H is defined, someone messed up and included ttvfs.h wrongly.
#endif

#include "VFSDefines.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <cassert>


#if _MSC_VER
# ifndef _CRT_SECURE_NO_WARNINGS
#   define _CRT_SECURE_NO_WARNINGS
# endif
#ifndef _CRT_SECURE_NO_DEPRECATE
#   define _CRT_SECURE_NO_DEPRECATE
#endif
#   pragma warning(disable: 4355) // 'this' : used in base member initializer list
#endif

template <typename DST, typename SRC> inline DST safecast(SRC p)
{
#ifndef NDEBUG
    assert(!p || static_cast<DST>(p) == dynamic_cast<DST>(p));
#endif
    return static_cast<DST>(p);
}

template <typename DST, typename SRC> inline DST safecastNonNull(SRC p)
{
#ifndef NDEBUG
    assert(p && static_cast<DST>(p) == dynamic_cast<DST>(p));
#endif
    return static_cast<DST>(p);
}


#endif
