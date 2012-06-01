// VFSInternal.h - misc things that are not required to be visible outside of the library.
// For conditions of distribution and use, see copyright notice in VFS.h

// !! this file is supposed to be included ONLY from VFS*.cpp files.

#ifndef VFS_INTERNAL_H
#define VFS_INTERNAL_H

// checks to enforce correct including
#ifdef TTVFS_VFS_H
#error Oops, TTVFS_VFS_H is defined, someone messed up and included VFS.h wrongly.
#endif

#include "VFSDefines.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <cassert>

VFS_NAMESPACE_START

inline char *allocHelper(allocator_func alloc, size_t size)
{
    return alloc ? (char*)alloc(size) : new char[size];
}

inline char *allocHelperExtra(allocator_func alloc, size_t size, size_t extra)
{
    char *p = (char*)allocHelper(alloc, size + extra);
    memset(p + size, 0, extra);
    return p;
}

template <typename T> inline void deleteHelper(delete_func deletor, T *mem)
{
    if(deletor)
        deletor(mem);
    else
        delete [] mem;
}

VFS_NAMESPACE_END


#if _MSC_VER
# ifndef _CRT_SECURE_NO_WARNINGS
#   define _CRT_SECURE_NO_WARNINGS
# endif
#ifndef _CRT_SECURE_NO_DEPRECATE
#   define _CRT_SECURE_NO_DEPRECATE
#endif
#   pragma warning(disable: 4355) // 'this' : used in base member initializer list
#endif


#endif
