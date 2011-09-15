// VFSLoader.cpp - late loading of files not in the tree
// For conditions of distribution and use, see copyright notice in VFS.h

#include "VFSInternal.h"
#include "VFSTools.h"
#include "VFSFile.h"
#include "VFSLoader.h"

VFS_NAMESPACE_START

#if !defined(_WIN32) && defined(VFS_IGNORE_CASE)

#include <sys/dir.h>

// based on code in PhysicsFS: http://icculus.org/physfs/
static bool locateOneElement(char *buf)
{
    char *ptr;
    DIR *dirp;

    ptr = strrchr(buf, '/');  // find entry at end of path.
    if (ptr == NULL)
    {
        dirp = opendir(".");
        ptr = buf;
    }
    else
    {
        *ptr = '\0';
        dirp = opendir(buf);
        *ptr = '/';
        ptr++;  // point past dirsep to entry itself.
    }

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

    printf("tt: Fixed case '%s' [%s]\n", fn, found ? "found" : "NOT FOUND"); // TEMP
    return found;
}
#endif


VFSFile *VFSLoaderDisk::Load(const char *fn)
{
    if(FileExists(fn))
        return new VFSFileReal(fn); // must contain full file name
    
#if !defined(_WIN32) && defined(VFS_IGNORE_CASE)
    size_t s = strlen(fn);
    if(s < 511) // avoid using malloc() and alloca() for short strings
    {
        char t[512];
        memcpy(&t[0], fn, s+1); // copy terminating '\0' as well
        if(findFileHarder(&t[0])) // fixes the filename on the way
            return new VFSFileReal(&t[0]);
    }
    else
    {
        char *t = (char*)malloc(s+1);
        VFSFileReal *vf = NULL;
        memcpy(t, fn, s+1);
        if(findFileHarder(&t[0]))
            vf = new VFSFileReal(&t[0]);
        free(t);
        return vf;
    }
#endif

    return NULL;
}

VFS_NAMESPACE_END
