// VFSDefines.h - compile config and basic setup
// For conditions of distribution and use, see copyright notice in VFS.h

#ifndef VFS_DEFINES_H
#define VFS_DEFINES_H

/* --- Config section -- modify as needed --- */

// choose a namespace name, or comment out to disable namespacing completely (not recommended)
#define VFS_NAMESPACE ttvfs

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

// Define this to make all VFSFile, VFSDir, VFSHelper operations thread-safe.
// If you do, do not forget to add your own implementation to VFSAtomic.cpp/.h !
// If this is not defined, you can still do manual locking if you know what you're doing,
// performance matters, and you implemented actual locking into the Mutex class.
// If no Mutex implementation is provided, its operations are no-ops, beware!
//#define VFS_THREADSAFE


/* --- End of config section --- */


#ifdef VFS_NAMESPACE
#    define VFS_NAMESPACE_START namespace VFS_NAMESPACE {
#    define VFS_NAMESPACE_END }
#    define VFS_NAMESPACE_IMPL VFS_NAMESPACE::
#else
#    define VFS_NAMESPACE_START
#    define VFS_NAMESPACE_END
#    define VFS_NAMESPACE_IMPL
#endif

VFS_NAMESPACE_START

#ifdef VFS_LARGEFILE_SUPPORT
#    if defined(_MSC_VER)
         typedef __int64           vfspos;
#    else
         typedef long long         vfspos;
#    endif
#else
    typedef unsigned int           vfspos;
#endif

// simple guard wrapper, works also if VFS_THREADSAFE is not defined
#define VFS_GUARD(obj) VFS_NAMESPACE_IMPL Guard __vfs_stack_guard((obj)->mutex())

// defines for optional auto-locking; only if VFS_THREADSAFE is defined
#ifdef VFS_THREADSAFE
#    define VFS_GUARD_OPT(obj) VFS_GUARD(obj)
#else
#    define VFS_GUARD_OPT(obj)
#endif

#if defined(_MSC_VER)
#    define VFS_STRICMP stricmp
#else
#    define VFS_STRICMP strcasecmp
#endif

static const vfspos npos = vfspos(-1);

VFS_NAMESPACE_END

#endif
