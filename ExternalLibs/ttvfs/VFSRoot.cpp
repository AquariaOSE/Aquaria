// VFSRoot.cpp - glues it all together and makes use simple
// For conditions of distribution and use, see copyright notice in VFS.h

#include "VFSInternal.h"
#include "VFSRoot.h"
#include "VFSTools.h"

#include "VFSDirInternal.h"
#include "VFSFile.h"
#include "VFSLoader.h"
#include "VFSArchiveLoader.h"
#include "VFSDirView.h"

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
    loadersInfo.clear();
}

void Root::Mount(const char *src, const char *dest)
{
    return AddVFSDir(GetDir(src, true), dest);
}

void Root::AddVFSDir(DirBase *dir, const char *subdir /* = NULL */)
{
    if(!subdir)
        subdir = dir->fullname();
    InternalDir *into = safecastNonNull<InternalDir*>(merged->_getDirEx(subdir, subdir, true, true, false).first);
    into->_addMountDir(dir);
}

bool Root::RemoveVFSDir(DirBase *dir, const char *subdir /* = NULL */)
{
    if(!subdir)
        subdir = dir->fullname();
    InternalDir *vddest = safecast<InternalDir*>(GetDir(subdir, false));
    if(!vddest)
        return false;

    vddest->_removeMountDir(dir);
    return true;
}

bool Root::Unmount(const char *src, const char *dest)
{
    DirBase *vdsrc = GetDir(src, false);
    if(!vdsrc)
        return false;

    return RemoveVFSDir(vdsrc, dest);
}

int Root::AddLoader(VFSLoader *ldr, const char *path /* = NULL */)
{
    DEBUG_ASSERT(ldr != NULL);
    loaders.push_back(ldr);
    loadersInfo.push_back(path);
    AddVFSDir(ldr->getRoot(), path);
    return (int)(loaders.size() - 1);
}

void Root::RemoveLoader(int index, const char *path /* = NULL */)
{
    VFSLoader *ldr = loaders[index];
    RemoveVFSDir(ldr->getRoot(), loadersInfo[index].getPath());
    loaders.erase(loaders.begin() + index);
    loadersInfo.erase(loadersInfo.begin() + index);
}

int Root::AddArchiveLoader(VFSArchiveLoader *ldr)
{
    DEBUG_ASSERT(ldr != NULL);
    archLdrs.push_back(ldr);
    return (int)(archLdrs.size() - 1);
}

void Root::RemoveArchiveLoader(int index)
{
    archLdrs.erase(archLdrs.begin() + index);
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
        ldr->getRoot()->add(vf);
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
        //ret = safecastNonNull<InternalDir*>(merged->_getDirEx(fn, fn, true, true, false).first);
        ret = safecastNonNull<InternalDir*>(merged->_createAndInsertSubtree(fn));
        ret->_addMountDir(realdir);
    }
    return ret;
}

DirBase *Root::GetDir(const char* dn, bool create /* = false */)
{
    //printf("Root  ::getDir [%s]\n", dn);
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
            vd = safecastNonNull<InternalDir*>(merged->_createAndInsertSubtree(dn)); // typecast is for debug checking only
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
    //printf("Root::FillDirView [%s]\n", path);
    return merged->fillView(path, view);
}


void Root::ClearGarbage()
{
    merged->clearGarbage();
}

bool Root::ForEach(const char *path, FileEnumCallback fileCallback /* = NULL */, DirEnumCallback dirCallback /* = NULL */,
                   void *user /* = NULL */, bool safe /* = false */)
{
    DirView view;
    if(!FillDirView(path, view))
        return false;

    if(dirCallback)
        view.forEachDir(dirCallback, user, safe);
    if(fileCallback)
        view.forEachFile(fileCallback, user, safe);

    return true;
}



VFS_NAMESPACE_END
