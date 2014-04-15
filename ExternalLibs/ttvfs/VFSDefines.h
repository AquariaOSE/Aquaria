// VFSDefines.h - compile config and basic setup
// For conditions of distribution and use, see copyright notice in VFS.h

#ifndef VFS_DEFINES_H
#define VFS_DEFINES_H

/* --- Config section -- modify as needed --- */

// Define this to allow dealing with files > 4 GB, using non-standard functions.
// This may or may not work with your platform/compiler, good luck.
//#define VFS_LARGEFILE_SUPPORT

// Define this to make all operations case insensitive.
// Windows systems generally don't care much, but for Linux and Mac this can be used
// to get the same behavior as on windows.
// Additionally, this achieves full case insensitivity within the library,
// if the the same files are accessed multiple times by the program, but with not-uniform case.
// (no sane programmer should do this, anyway).
// However, on non-windows systems this will decrease performance when checking for files
// on disk (see VFSLoader.cpp).
#define VFS_IGNORE_CASE


/* --- End of config section --- */


#define VFS_NAMESPACE_START namespace ttvfs {
#define VFS_NAMESPACE_END }


#if !defined(_MSC_VER)
#  include <stdint.h>
#endif

VFS_NAMESPACE_START

#ifdef VFS_LARGEFILE_SUPPORT
#    if defined(_MSC_VER)
         typedef __int64           vfspos;
#    else
#        include <stdint.h>
         typedef int64_t           vfspos;
#    endif
#else
    typedef unsigned int           vfspos;
#endif

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#    define VFS_STRICMP _stricmp
     static const vfspos npos = vfspos(-1i64);
#else
#    define VFS_STRICMP strcasecmp
     static const vfspos npos = vfspos(-1LL);
#endif

typedef void (*delete_func)(void *);

struct _AbiCheck
{
    int structSize;
    int vfsposSize;
    int largefile;
    int nocase;
};


VFS_NAMESPACE_END

#endif
