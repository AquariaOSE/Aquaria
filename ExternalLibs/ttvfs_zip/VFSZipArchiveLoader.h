#ifndef VFS_ZIP_ARCHIVE_LOADER_H
#define VFS_ZIP_ARCHIVE_LOADER_H

#include "VFSArchiveLoader.h"

VFS_NAMESPACE_START

class VFSZipArchiveLoader : public VFSArchiveLoader
{
public:
    virtual ~VFSZipArchiveLoader() {}
    virtual VFSDir *Load(VFSFile *arch, VFSLoader **ldr, void *opaque = NULL);
};

VFS_NAMESPACE_END

#endif
