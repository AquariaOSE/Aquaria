// VFSLoader.cpp - late loading of files not in the tree
// For conditions of distribution and use, see copyright notice in VFS.h

#include "VFSInternal.h"
#include "VFSTools.h"
#include "VFSFile.h"
#include "VFSDir.h"
#include "VFSLoader.h"

VFS_NAMESPACE_START

#if !defined(_WIN32) && defined(VFS_IGNORE_CASE)

#include <dirent.h>

// based on code in PhysicsFS: http://icculus.org/physfs/
static bool locateOneElement(char *buf)
{
    char *ptr;
    DIR *dirp;

    ptr = strrchr(buf, '/');  // find entry at end of path.

    //printf("locateOneElem: buf='%s' ptr='%s'\n", ptr, buf);

    if (ptr == NULL)
    {
        dirp = opendir(".");
        ptr = buf;
    }
    else
    {
        if(ptr != buf) // strip only if not abs path
            *ptr = '\0';
        //printf("opendir: '%s'\n", buf);
        dirp = opendir(buf);
        *ptr = '/';
        ptr++;  // point past dirsep to entry itself.
    }

    //printf("dirp = %p\n", dirp);

    struct dirent *dent;
    while ((dent = readdir(dirp)) != NULL)
    {
        if (strcasecmp(dent->d_name, ptr) == 0)
        {
            strcpy(ptr, dent->d_name); // found a match. Overwrite with this case.
            closedir(dirp);
            return true;
        }
    }

    // no match at all...
    closedir(dirp);
    return false;
}

static bool findFileHarder(char *fn)
{
    char *ptr = fn;
    bool found = true;
    while ((ptr = strchr(ptr + 1, '/')) != 0)
    {
        *ptr = '\0';
        found = locateOneElement(fn);
        *ptr = '/'; // restore path separator
        if (!found)
            return false;
    }

    // check final element...
    found = found && locateOneElement(fn);

    //printf("tt: Fixed case '%s' [%s]\n", fn, found ? "found" : "NOT FOUND"); // TEMP
    return found;
}
#endif


VFSFile *VFSLoaderDisk::Load(const char *fn, const char * /*ignored*/)
{
    if(FileExists(fn))
        return new VFSFileReal(fn); // must contain full file name

    VFSFileReal *vf = NULL;

#if !defined(_WIN32) && defined(VFS_IGNORE_CASE)
    size_t s = strlen(fn);
    char *t = (char*)VFS_STACK_ALLOC(s+1);
    memcpy(t, fn, s+1); // copy terminating '\0' as well
    if(findFileHarder(&t[0])) // fixes the filename on the way
        vf = new VFSFileReal(&t[0]);
    VFS_STACK_FREE(t);
#endif

    return vf;
}

VFSDir *VFSLoaderDisk::LoadDir(const char *fn, const char * /*ignored*/)
{
    if(IsDirectory(fn))
        return new VFSDirReal(fn); // must contain full file name

    VFSDirReal *ret = NULL;

#if !defined(_WIN32) && defined(VFS_IGNORE_CASE)
    size_t s = strlen(fn);
    char *t = (char*)VFS_STACK_ALLOC(s+1);
    memcpy(t, fn, s+1); // copy terminating '\0' as well
    if(findFileHarder(&t[0])) // fixes the filename on the way
    {
        ret = new VFSDirReal(&t[0]);
    }
    VFS_STACK_FREE(t);
#endif

    return ret;
}

VFS_NAMESPACE_END
