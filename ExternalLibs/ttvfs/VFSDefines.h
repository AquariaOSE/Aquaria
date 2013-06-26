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
// Note: This adds a *lot* of overhead. Better ensure thread safety yourself, externally. Really!
// (Also note that this feature is *UNTESTED*. Don't activate.)
//#define VFS_THREADSAFE

// By default, ttvfs uses a std::map to store stuff.
// Uncomment the line below to use an (experimental!) hashmap.
// With std::map, iterating over entries will always deliver them in sorted order.
// The hashmap will deliver entries in random order, but lookup will be much faster
// (especially if VFS_IGNORE_CASE is set!)
//#define VFS_USE_HASHMAP

// These are used for small, temporary memory allocations that can remain on the stack.
// If alloca is available, this is the preferred way.
#include <stdlib.h>
#ifdef _WIN32
#  include <malloc.h> // MSVC/MinGW still need this for alloca. Seems to be windows-specific failure
#endif
#define VFS_STACK_ALLOC(size) alloca(size)
#define VFS_STACK_FREE(ptr)   /* no need to free anything here */
// Fail-safe:
//#define VFS_STACK_ALLOC(size) malloc(size)
//#define VFS_STACK_FREE(ptr)  free(ptr)

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

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#    define VFS_STRICMP stricmp
#else
#    define VFS_STRICMP strcasecmp
#endif

static const vfspos npos = vfspos(-1);

typedef void *(*allocator_func)(size_t);
typedef void (*delete_func)(void*);

VFS_NAMESPACE_END

#endif
