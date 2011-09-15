// VFSInternal.h - misc things that are not required to be visible outside of the library.
// For conditions of distribution and use, see copyright notice in VFS.h

// !! this file is supposed to be included ONLY from VFS*.cpp files.

#ifndef VFS_INTERNAL_H
#define VFS_INTERNAL_H

// checks to enforcecorrect including
#ifdef TTVFS_VFS_H
#error Oops, TTVFS_VFS_H is defined, someone messed up and included VFS.h wrongly.
#endif

#include "VFSDefines.h"

#if _MSC_VER
#   define _CRT_SECURE_NO_WARNINGS
#   define _CRT_SECURE_NO_DEPRECATE
#   pragma warning(disable: 4355) // 'this' : used in base member initializer list
#endif

// this is for POSIX - define before including any stdio headers
#ifdef VFS_LARGEFILE_SUPPORT
#    ifndef _MSC_VER
#        define _FILE_OFFSET_BITS 64
#    endif
#endif


#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>

// this is for MSVC - re-define functions
#ifdef VFS_LARGEFILE_SUPPORT
#    ifdef _MSC_VER
#        define fseek _fseeki64
#        define ftell _ftelli64
#    endif
#endif

#endif
