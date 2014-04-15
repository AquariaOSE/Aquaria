#include "VFSInternal.h"
#include "VFSZipArchiveLoader.h"
#include "VFSDirZip.h"
#include "VFSZipArchiveRef.h"

VFS_NAMESPACE_START

Dir *VFSZipArchiveLoader::Load(File *arch, VFSLoader ** /*unused*/, void * /*unused*/)
{
    CountedPtr<ZipArchiveRef> zref = new ZipArchiveRef(arch);
    if(!zref->init() || !zref->openRead())
        return NULL;
    ZipDir *vd = new ZipDir(zref, arch->fullname(), true);
    vd->load();
    return vd;
}

VFS_NAMESPACE_END
