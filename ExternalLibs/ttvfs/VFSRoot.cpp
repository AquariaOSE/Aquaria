// VFSRoot.cpp - glues it all together and makes use simple
// For conditions of distribution and use, see copyright notice in VFS.h

#include <iostream> // for debug only, see EOF

#include "VFSInternal.h"
#include "VFSRoot.h"
#include "VFSTools.h"

#include "VFSDirInternal.h"
#include "VFSFile.h"
#include "VFSLoader.h"
#include "VFSArchiveLoader.h"

#ifdef _DEBUG
#  include <cassert>
#  define DEBUG_ASSERT(x) assert(x)
#else
#  define DEBUG_ASSERT(x)
#endif


VFS_NAMESPACE_START

// predecl is in VFS.h
bool _checkCompatInternal(_AbiCheck *used)
{
    if(sizeof(_AbiCheck) != used->structSize)
        return false;

    _AbiCheck here;
    memset(&here, 0, sizeof(here));
    here.structSize = sizeof(here);
    here.vfsposSize = sizeof(vfspos);

#ifdef VFS_LARGEFILE_SUPPORT
    here.largefile = 1;
#endif

#ifdef VFS_IGNORE_CASE
    here.nocase = 1;
#endif

    return !memcmp(&here, used, sizeof(here));
}

Root::Root()
: merged(new InternalDir(""))
{
}

Root::~Root()
{
}

void Root::Clear()
{
    merged->_clearDirs();
    merged->_clearMounts();

    loaders.clear();
    archLdrs.clear();
}

void Root::Mount(const char *src, const char *dest)
{
    return AddVFSDir(GetDir(src, true), dest);
}

void Root::AddVFSDir(DirBase *dir, const char *subdir /* = NULL */)
{
    if(!subdir)
        subdir = dir->fullname();
    InternalDir *into = safecastNonNull<InternalDir*>(merged->getDir(subdir, true, true, false));
    into->_addMountDir(dir);
}

bool Root::Unmount(const char *src, const char *dest)
{
    DirBase *vdsrc = GetDir(src, false);
    InternalDir *vddest = safecast<InternalDir*>(GetDir(dest, false));
    if(!vdsrc || !vddest)
        return false;

    vddest->_removeMountDir(vdsrc);
    return true;
}

void Root::AddLoader(VFSLoader *ldr, const char *path /* = NULL */)
{
    loaders.push_back(ldr);
    AddVFSDir(ldr->getRoot(), path);
}

void Root::AddArchiveLoader(VFSArchiveLoader *ldr)
{
    archLdrs.push_back(ldr);
}
Dir *Root::AddArchive(const char *arch, void *opaque /* = NULL */)
{
    return AddArchive(GetFile(arch), arch, opaque);
}

Dir *Root::AddArchive(File *file, const char *path /* = NULL */, void *opaque /* = NULL */)
{
    if(!file || !file->open("rb"))
        return NULL;

    Dir *ad = NULL;
    VFSLoader *fileLdr = NULL;
    for(ArchiveLoaderArray::iterator it = archLdrs.begin(); it != archLdrs.end(); ++it)
        if((ad = (*it)->Load(file, &fileLdr, opaque)))
            break;
    if(!ad)
        return NULL;

    if(fileLdr)
        loaders.push_back(fileLdr);

    AddVFSDir(ad, path ? path : file->fullname());

    return ad;
}

inline static File *VFSHelper_GetFileByLoader(VFSLoader *ldr, const char *fn, const char *unmangled)
{
    if(!ldr)
        return NULL;
    File *vf = ldr->Load(fn, unmangled);
    if(vf)
        ldr->getRoot()->addRecursive(vf);
    return vf;
}

File *Root::GetFile(const char *fn)
{
    const char *unmangled = fn;
    std::string fixed = fn; // TODO: get rid of allocation here
    FixPath(fixed);
    fn = fixed.c_str();

    File *vf = NULL;

    vf = merged->getFile(fn);

    // nothing found? maybe a loader has something.
    // if so, add the newly created File to the tree.
    if(!vf)
        for(LoaderArray::iterator it = loaders.begin(); it != loaders.end(); ++it)
            if((vf = VFSHelper_GetFileByLoader(*it, fn, unmangled)))
                break;

    //printf("VFS: GetFile '%s' -> '%s' (%s:%p)\n", fn, vf ? vf->fullname() : "NULL", vf ? vf->getType() : "?", vf);

    return vf;
}

InternalDir *Root::_GetDirByLoader(VFSLoader *ldr, const char *fn, const char *unmangled)
{
    Dir *realdir = ldr->LoadDir(fn, unmangled);
    InternalDir *ret = NULL;
    if(realdir)
    {
        ret = safecastNonNull<InternalDir*>(merged->getDir(fn, true, true, false));
        ret->_addMountDir(realdir);
    }
    return ret;
}

DirBase *Root::GetDir(const char* dn, bool create /* = false */)
{
    const char *unmangled = dn;
    std::string fixed = dn; // TODO: get rid of alloc
    FixPath(fixed);
    dn = fixed.c_str();

    if(!*dn)
        return merged;
    DirBase *vd = merged->getDir(dn);

    if(!vd)
    {
        for(LoaderArray::iterator it = loaders.begin(); it != loaders.end(); ++it)
            if((vd = _GetDirByLoader(*it, dn, unmangled)))
                break;

        if(!vd && create)
            vd = safecastNonNull<InternalDir*>(merged->getDir(dn, true)); // typecast is for debug checking only
    }

    //printf("VFS: GetDir '%s' -> '%s' (%s:%p)\n", dn, vd ? vd->fullname() : "NULL", vd ? vd->getType() : "?", vd);

    return vd;
}

DirBase *Root::GetDirRoot()
{
    return merged;
}

bool Root::FillDirView(const char *path, DirView& view)
{
    return merged->fillView(path, view);
}


void Root::ClearGarbage()
{
    merged->clearGarbage();
}



// DEBUG STUFF


struct _DbgParams
{
    _DbgParams(std::ostream& os_, DirBase *parent_, const std::string& sp_)
        : os(os_), parent(parent_), sp(sp_) {}

    std::ostream& os;
    DirBase *parent;
    const std::string& sp;
};

static void _DumpFile(File *vf, void *user)
{
    _DbgParams& p = *((_DbgParams*)user);

    p.os << p.sp << "f|" << vf->name() << " [" << vf->getType() << ", ref " << vf->getRefCount() << ", 0x" << vf << "]";

    if(strncmp(p.parent->fullname(), vf->fullname(), p.parent->fullnameLen()))
        p.os << " <-- {" << vf->fullname() << "} ***********";

    p.os << std::endl;
}

static void _DumpTreeRecursive(DirBase *vd, void *user)
{
    _DbgParams& p = *((_DbgParams*)user);

    std::string sub = p.sp + "  ";

    p.os << p.sp << "d|" << vd->name() << " [" << vd->getType() << ", ref " << vd->getRefCount() << ", 0x" << vd << "]";

    if(p.parent && strncmp(p.parent->fullname(), vd->fullname(), strlen(p.parent->fullname())))
        p.os << " <-- {" << vd->fullname() << "} ***********";
    p.os << std::endl;

    _DbgParams recP(p.os, vd, sub);

    vd->forEachDir(_DumpTreeRecursive, &recP);

    vd->forEachFile(_DumpFile, &recP);

}

void Root::debugDumpTree(std::ostream& os, Dir *start /* = NULL */)
{
    _DbgParams recP(os, NULL, "");
    DirBase *d = start ? start : GetDirRoot();
    _DumpTreeRecursive(d, &recP);
}


VFS_NAMESPACE_END
