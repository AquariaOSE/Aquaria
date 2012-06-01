#include "VFSInternal.h"
#include "VFSZipArchiveLoader.h"
#include "VFSDirZip.h"

VFS_NAMESPACE_START

VFSDir *VFSZipArchiveLoader::Load(VFSFile *arch, VFSLoader ** /*unused*/, void * /*unused*/)
{
    VFSDirZip *vd = new VFSDirZip(arch);
    if(vd->load(true))
        return vd;

    vd->ref--;
    return NULL;
}

VFS_NAMESPACE_END
