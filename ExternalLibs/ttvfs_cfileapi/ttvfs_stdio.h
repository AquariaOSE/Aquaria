#ifndef TTVFS_STDIO_OVERRIDE_H
#define TTVFS_STDIO_OVERRIDE_H

// Before including this file:
// * Define VFS_ENABLE_C_API to 0 to use the normal functions (fopen() and friends, std::ifstream).
// * Define VFS_ENABLE_C_API to 1 to use ttvfs overrides.

/*
    This file is a poor man's wrapper to replace the C API and std::ifstream.
    Note that if you have any advanced needs, this wrapper API is not for you.

    To use it, go through your code and rename all FILE* to VFILE*,
    and fopen() and related to vfopen() (so just put a 'v' in front).
    Instead of std::ifstream, use InStream. If you use std::fstream for reading ONLY,
    also use InStream.
    Make sure that a FILE* is not opened twice at any time - this is not supported.

    Note that the seek and tell functions do not offer 64 bit offsets in this API.
*/


//-------------------------------------------------
#if VFS_ENABLE_C_API-0 == 1

#include <iostream>
#include <sstream>
#include <stdio.h>

namespace ttvfs
{
    class File;
    class Root;
}
typedef ttvfs::File VFILE;


void ttvfs_setroot(ttvfs::Root *root);

// HACK
VFILE *vfgetfile(const char *fn);

// Note that vfopen() returns the same pointer for the same file name,
// so effectively a file is a singleton object.
VFILE *vfopen(const char *fn, const char *mode);
size_t vfread(void *ptr, size_t size, size_t count, VFILE *vf);
int vfclose(VFILE *vf);
size_t vfwrite(const void *ptr, size_t size, size_t count, VFILE *vf);
int vfseek(VFILE *vf, long int offset, int origin);
char *vfgets(char *str, int num, VFILE *vf);
long int vftell(VFILE *vf);
int vfeof(VFILE *vf);
int vfsize(VFILE *vf, size_t *sizep); // extension

// This class is a minimal adapter to support STL-like read-only file streams for VFS files, using std::istringstream.
// It loads the whole file content at once. If this is a problem, roll your own, or use the lower-level ttvfs::File functions.
class InStream : public std::istringstream
{
public:
    InStream(const char *fn);
    InStream(const std::string& fn);
    bool open(const char *fn);
    inline bool is_open() { return good(); }
    void close() {}
private:
    void _init(const char *fn);
};

#define VFS_USING_C_API 1

//--------------------------------------------------
#elif VFS_ENABLE_C_API-0 == 0

#include <stdio.h>
#include <iosfwd>
typedef std::ifstream InStream;
typedef FILE VFILE;

int ttvfs_stdio_fsize(VFILE *f, size_t *sizep); // extension

#define vfopen        fopen
#define vfread        fread
#define vfclose       fclose
#define vfwrite       fwrite
#define vfseek        fseek
#define vfgets        fgets
#define vftell        ftell
#define vfeof         feof
#define vfsize        ttvfs_stdio_fsize

//-------------------------------------------------
#else // VFS_ENABLE_C_API
   // Help catch errors or forgotten defines if this file is included
#  error #define VFS_ENABLE_C_API to either 0 or 1
#endif


#endif // FILE_API_H
