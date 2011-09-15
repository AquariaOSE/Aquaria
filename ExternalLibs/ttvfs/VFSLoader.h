// VFSLoader.h - late loading of files not in the tree
// For conditions of distribution and use, see copyright notice in VFS.h

#ifndef VFSLOADER_H
#define VFSLOADER_H

#include "VFSDefines.h"

VFS_NAMESPACE_START

class VFSFile;

// VFSLoader - to be called if a file is not in the tree.
class VFSLoader
{
public:
    virtual ~VFSLoader() {}
    virtual VFSFile *Load(const char *fn) = 0;
};

class VFSLoaderDisk : public VFSLoader
{
public:
    virtual ~VFSLoaderDisk() {}
    virtual VFSFile *Load(const char *fn);
};

VFS_NAMESPACE_END


#endif