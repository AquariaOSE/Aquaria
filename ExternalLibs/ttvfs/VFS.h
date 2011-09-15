/* ttvfs -- tiny tree virtual file system

// VFS.h - all the necessary includes to get a basic VFS working
// Only include externally, not inside the library.

See VFSDefines.h for compile configration.


---------[ License ]----------
MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


#ifndef TTVFS_VFS_H
#define TTVFS_VFS_H

#include "VFSDefines.h"

VFS_NAMESPACE_START
bool _checkCompatInternal(bool large, bool nocase, unsigned int vfspos_size);

/** It is recommended to call this function early in your code
    and ensure it returns true - if it does not, compiler settings
    are inconsistent, which may cause otherwise hard to detect problems. */
inline static bool checkCompat(void)
{
#ifdef VFS_LARGEFILE_SUPPORT
    bool largefile = true;
#else
    bool largefile = false;
#endif

#ifdef VFS_IGNORE_CASE
    bool nocase = true;
#else
    bool nocase = false;
#endif
    return _checkCompatInternal(largefile, nocase, sizeof(vfspos));
}
VFS_NAMESPACE_END


#include <cstring>
#include <string>
#include "VFSHelper.h"
#include "VFSFile.h"
#include "VFSDir.h"


// Checks to enforce correct including.
// At least on windows, <string> includes <cstdio>,
// but that must be included after "VFSInternal.h",
// and "VFSInternal.h" may only be used inside the library (or by extensions),
// because it redefines fseek and ftell, which would
// mess up the ABI if included elsewhere.
#ifdef VFS_INTERNAL_H
#error Oops, VFS_INTERNAL_H is defined, someone messed up and included VFSInternal.h wrongly.
#endif

#endif
