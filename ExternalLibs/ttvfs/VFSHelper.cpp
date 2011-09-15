// VFSHelper.cpp - glues it all together and makes use simple
// For conditions of distribution and use, see copyright notice in VFS.h

#include <iostream> // for debug only, see EOF

#include "VFSInternal.h"
#include "VFSHelper.h"
#include "VFSAtomic.h"
#include "VFSTools.h"

#include "VFSDir.h"
#include "VFSFile.h"
#include "VFSLoader.h"

#ifdef _DEBUG
#  include <cassert>
#  define DEBUG_ASSERT(x) assert(x)
#else
#  define DEBUG_ASSERT(x)
#endif


VFS_NAMESPACE_START

// predecl is in VFS.h
bool _checkCompatInternal(bool large, bool nocase, unsigned int vfspos_size)
{
#ifdef VFS_LARGEFILE_SUPPORT
    bool largefile_i = true;
#else
    bool largefile_i = false;
#endif

#ifdef VFS_IGNORE_CASE
    bool nocase_i = true;
#else
    bool nocase_i = false;
#endif

    return (large == largefile_i)
        && (nocase == nocase_i)
        && (sizeof(vfspos) == vfspos_size);
}

VFSHelper::VFSHelper()
: filesysRoot(NULL), merged(NULL)
{
    _ldrDiskId = _AddFixedLoader(); // NULL intentionally. created by LoadFileSysRoot()
}

VFSHelper::~VFSHelper()
{
    Clear();
}

void VFSHelper::Clear(void)
{
    VFS_GUARD_OPT(this);
    _cleanup();

    if(filesysRoot)
    {
        filesysRoot->ref--; // this should always delete it...
        filesysRoot = NULL; // ...but it may be referenced elsewhere, just in case
    }

    for(unsigned int i = 0; i < preRoot.size(); ++i)
        preRoot[i]->ref--;
    preRoot.clear();

    for(unsigned int i = 0; i < postRoot.size(); ++i)
        postRoot[i]->ref--;
    postRoot.clear();

    for(unsigned int i = 0; i < FixedLoadersCount(); ++i)
        if(fixedLdrs[i])
        {
            delete fixedLdrs[i];
            fixedLdrs[i] = NULL;
        }
}

unsigned int VFSHelper::_AddFixedLoader(VFSLoader *ldr /* = NULL */)
{
    fixedLdrs.push_back(ldr);
    return FixedLoadersCount() - 1;
}

void VFSHelper::_cleanup(void)
{
    VFS_GUARD_OPT(this); // be extra safe and ensure this is locked
    if(merged)
    {
        merged->ref--;
        merged = NULL;
    }
    for(VFSMountList::iterator it = vlist.begin(); it != vlist.end(); ++it)
        it->vdir->ref--;
    vlist.clear();
    for(LoaderList::iterator it = dynLdrs.begin(); it != dynLdrs.end(); ++it)
        delete *it;
    dynLdrs.clear();
}

bool VFSHelper::LoadFileSysRoot(void)
{
    VFS_GUARD_OPT(this);
    VFSDirReal *oldroot = filesysRoot;

    filesysRoot = new VFSDirReal;
    if(!filesysRoot->load("."))
    {
        filesysRoot->ref--;
        filesysRoot = oldroot;
        return false;
    }

    if(!fixedLdrs[_ldrDiskId])
        fixedLdrs[_ldrDiskId] = new VFSLoaderDisk;

    if(oldroot)
        oldroot->ref--;

    return true;
}

void VFSHelper::Prepare(bool clear /* = true */)
{
    VFS_GUARD_OPT(this);
    if(clear)
        _cleanup();
    if(!merged)
    {
        merged = new VFSDir("");
    }
    
    for(size_t i = 0; i < preRoot.size(); ++i)
        merged->merge(preRoot[i]);
    if(filesysRoot)
        merged->merge(filesysRoot);
    for(size_t i = 0; i < postRoot.size(); ++i)
        merged->merge(postRoot[i]);
}

void VFSHelper::Reload(bool fromDisk /* = false */)
{
    VFS_GUARD_OPT(this);
    if(fromDisk)
        LoadFileSysRoot();
    Prepare(false);
    for(VFSMountList::iterator it = vlist.begin(); it != vlist.end(); it++)
        GetDir(it->mountPoint.c_str(), true)->merge(it->vdir, it->overwrite);
}

bool VFSHelper::Mount(const char *src, const char *dest, bool overwrite /* = true*/)
{
    VFS_GUARD_OPT(this);
    return AddVFSDir(GetDir(src, false), dest, overwrite);
}

bool VFSHelper::AddVFSDir(VFSDir *dir, const char *subdir /* = NULL */, bool overwrite /* = true */)
{
    if(!dir)
        return false;
    VFS_GUARD_OPT(this);
    if(!subdir)
        subdir = dir->fullname();
    VFSDir *sd = GetDir(subdir, true);
    if(!sd) // may be NULL if Prepare() was not called before
        return false;
    dir->ref++; // because this is to be added to vlist
    VDirEntry ve(dir, subdir, overwrite);
    _StoreMountPoint(ve);
    sd->merge(dir, overwrite); // merge into specified subdir. will be (virtually) created if not existing
    return true;
}

bool VFSHelper::Unmount(const char *src, const char *dest)
{
    VFSDir *vd = GetDir(src, false);
    if(!vd)
        return false;

    VDirEntry ve(vd, dest, true); // last is dummy
    if(!_RemoveMountPoint(ve))
        return false;

    Reload(false);
    return true;
}

void VFSHelper::_StoreMountPoint(const VDirEntry& ve)
{
    // scan through and ensure only one mount point with the same data is present.
    // if present, remove and re-add, this ensures the mount point is at the end of the list
    for(VFSMountList::iterator it = vlist.begin(); it != vlist.end(); )
    {
        const VDirEntry& oe = *it;
        if(ve.mountPoint == oe.mountPoint && ve.vdir == oe.vdir
            && (ve.overwrite || !oe.overwrite) ) // overwrite definitely, or if other does not overwrite
        {
            it = vlist.erase(it); // do not break; just in case there are more (fixme?)
        }
        else
            ++it;
    }

    vlist.push_back(ve);
}

bool VFSHelper::_RemoveMountPoint(const VDirEntry& ve)
{
    for(VFSMountList::iterator it = vlist.begin(); it != vlist.end(); ++it)
    {
        const VDirEntry& oe = *it;
        if(ve.mountPoint == oe.mountPoint && ve.vdir == oe.vdir)
        {
            vlist.erase(it);
            return true;
        }
    }
    return false;
}

bool VFSHelper::MountExternalPath(const char *path, const char *where /* = "" */)
{
    // no guard required here, AddVFSDir has one, and the reference count is locked as well.
    VFSDirReal *vfs = new VFSDirReal;
    if(vfs->load(path))
        AddVFSDir(vfs, where);
    return !!--(vfs->ref); // 0 if deleted
}

void VFSHelper::AddLoader(VFSLoader *ldr)
{
    VFS_GUARD_OPT(this);
    dynLdrs.push_back(ldr);
}

inline static VFSFile *VFSHelper_GetFileByLoader(VFSLoader *ldr, const char *fn, VFSDir *root)
{
    if(!ldr)
        return NULL;
    VFSFile *vf = ldr->Load(fn);
    if(vf)
    {
        root->addRecursive(vf, true);
        --(vf->ref);
    }
    return vf;
}

VFSFile *VFSHelper::GetFile(const char *fn)
{
    while(fn[0] == '.' && fn[1] == '/')
        fn += 2;

    VFSFile *vf = NULL;

    // guarded block
    {
        VFS_GUARD_OPT(this);

        if(!merged) // Prepare() called?
            return NULL;

        vf = merged->getFile(fn);
    }

    // nothing found? maybe a loader has something.
    // if so, add the newly created VFSFile to the tree.
    // constant, no locking required here - also a bad idea in case a loader does heavy I/O
    if(!vf)
        for(unsigned int i = 0; i < fixedLdrs.size(); ++i)
            if((vf = VFSHelper_GetFileByLoader(fixedLdrs[i], fn, GetDirRoot())))
                break;

    if(!vf)
    {
        VFS_GUARD_OPT(this);
        for(LoaderList::iterator it = dynLdrs.begin(); it != dynLdrs.end(); ++it)
            if((vf = VFSHelper_GetFileByLoader(*it, fn, GetDirRoot())))
                break;
    }

    //printf("VFS: GetFile '%s' -> '%s' (%p)\n", fn, vf ? vf->fullname() : "NULL", vf);

    return vf;
}

VFSDir *VFSHelper::GetDir(const char* dn, bool create /* = false */)
{
    while(dn[0] == '.' && dn[1] == '/')
        dn += 2;

    VFS_GUARD_OPT(this);
    return (merged && *dn) ? merged->getDir(dn, create) : merged;
}

VFSDir *VFSHelper::GetDirRoot(void)
{
    VFS_GUARD_OPT(this);
    return merged;
}


// DEBUG STUFF

static void _DumpTreeRecursive(std::ostream& os, VFSDir *vd, const std::string& sp, VFSDir *parent)
{
    std::string sub = sp + "  ";

    os << sp << "d|" << vd->name() << " [" << vd->getType() << ", ref " << vd->ref.count() << ", 0x" << vd << "]";

    if(parent && strncmp(parent->fullname(), vd->fullname(), strlen(parent->fullname())))
        os << " <-- {" << vd->fullname() << "} ***********";
    os << std::endl;

    for(DirIter it = vd->_subdirs.begin(); it != vd->_subdirs.end(); ++it)
        _DumpTreeRecursive(os, it->second, sub, vd);

    for(FileIter it = vd->_files.begin(); it != vd->_files.end(); ++it)
    {
        VFSFile *vf = it->second;
        // only if refcount and/or mount point differs
        bool p = false;
        if(vf->ref.count() != vd->ref.count())
        {
            doprint:
            os << sub << "f|" << vf->name() << " [" << vf->getType() << ", ref " << vf->ref.count() << ", 0x" << vf << "]";
            p = true;
        }
        if(strncmp(vd->fullname(), vf->fullname(), strlen(vd->fullname())))
        {
            if(!p)
                goto doprint;
            os << " <-- {" << vf->fullname() << "} ***********";
        }
        if(p)
            os << std::endl;
    }
}

void VFSHelper::debugDumpTree(std::ostream& os, VFSDir *start /* = NULL */)
{
    _DumpTreeRecursive(os, start ? start : GetDirRoot(), "", NULL);
}


VFS_NAMESPACE_END
