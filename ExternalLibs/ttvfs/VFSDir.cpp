// Dir.cpp - basic directory interface + classes
// For conditions of distribution and use, see copyright notice in VFS.h

#include <set>

#include "VFSInternal.h"
#include "VFSTools.h"
#include "VFSFile.h"
#include "VFSDir.h"
#include "VFSDirView.h"
#include "VFSLoader.h"

VFS_NAMESPACE_START

DirBase::DirBase(const char *fullpath)
{
    _setName(fullpath);
}

DirBase::~DirBase()
{
}

File *DirBase::getFile(const char *fn)
{
    while(fn[0] == '.' && fn[1] == '/')
        fn += 2;

    const char *slashpos = strchr(fn, '/');
    if(!slashpos)
        return getFileByName(fn, true);

    const size_t len = slashpos - fn;
    char *dirname = (char*)VFS_STACK_ALLOC(len + 1);
    memcpy(dirname, fn, len);
    dirname[len] = 0;

    File *f = getFileFromSubdir(dirname, slashpos + 1);
    VFS_STACK_FREE(dirname);
    return f;
}

DirBase *DirBase::getDir(const char *subdir, bool forceCreate /* = false */, bool lazyLoad /* = true */, bool useSubtrees /* = true */)
{
    SkipSelfPath(subdir);
    if(!subdir[0])
        return this;

    DirBase *ret = NULL;
    char *slashpos = (char *)strchr(subdir, '/');

    // if there is a '/' in the string, descend into subdir and continue there
    if(slashpos)
    {
        // from a/b/c, cut out the a, without the trailing '/'.
        const char *sub = slashpos + 1;
        size_t copysize = slashpos - subdir;
        char * const t = (char*)VFS_STACK_ALLOC(copysize + 1);
        memcpy(t, subdir, copysize);
        t[copysize] = 0;
        
        if(DirBase *dir = getDirByName(t, lazyLoad, useSubtrees))
        {
            // TODO: get rid of recursion
            ret = dir->getDir(sub, forceCreate, lazyLoad); // descend into subdirs
        }
        else if(forceCreate)
        {
            // -> newname = fullname() + '/' + t
            size_t fullLen = fullnameLen();
            DirBase *ins;
            if(fullLen)
            {
                char * const newname = (char*)VFS_STACK_ALLOC(fullLen + copysize + 2);
                char *ptr = newname;
                memcpy(ptr, fullname(), fullLen);
                ptr += fullLen;
                *ptr++ = '/';
                memcpy(ptr, t, copysize);
                ptr[copysize] = 0;
                ins = createNew(newname);
                VFS_STACK_FREE(newname);
            }
            else
                ins = createNew(t);

            _subdirs[ins->name()] = ins;
            ret = ins->getDir(sub, true, lazyLoad); // create remaining structure
        }
    }
    else
    {
        if(DirBase *dir = getDirByName(subdir, lazyLoad, useSubtrees))
            ret = dir;
        else if(forceCreate)
        {
            size_t fullLen = fullnameLen();
            if(fullLen)
            {
                // -> newname = fullname() + '/' + subdir
                size_t subdirLen = strlen(subdir);
                char * const newname = (char*)VFS_STACK_ALLOC(fullLen + subdirLen + 2);
                char *ptr = newname;
                memcpy(ptr, fullname(), fullLen);
                ptr += fullLen;
                *ptr++ = '/';
                memcpy(ptr, subdir, subdirLen);
                ptr[subdirLen] = 0;

                ret = createNew(newname);
                VFS_STACK_FREE(newname);
            }
            else
            {
                ret = createNew(subdir);
            }

            _subdirs[ret->name()] = ret;
        }
    }

    return ret;
}

static void _iterDirs(Dirs &m, DirEnumCallback f, void *user)
{
    for(Dirs::iterator it = m.begin(); it != m.end(); ++it)
        f(it->second.content(), user);
}

void DirBase::forEachDir(DirEnumCallback f, void *user /* = NULL */, bool safe /* = false */)
{
    if(safe)
    {
        Dirs cp = _subdirs;
        _iterDirs(cp, f, user);
    }
    else
        _iterDirs(_subdirs, f, user);
}

DirBase *DirBase::getDirByName(const char *dn, bool /* unused: lazyLoad = true */, bool useSubtrees /* = true */)
{
    Dirs::iterator it = _subdirs.find(dn);
    return it != _subdirs.end() ? it->second : NULL;
}

void DirBase::clearGarbage()
{
    for(Dirs::iterator it = _subdirs.begin(); it != _subdirs.end(); ++it)
        it->second->clearGarbage();
}



Dir::Dir(const char *fullpath, VFSLoader *ldr)
: DirBase(fullpath), _loader(ldr)
{
}

Dir::~Dir()
{
}

File *Dir::getFileByName(const char *fn, bool lazyLoad /* = true */)
{
    Files::iterator it = _files.find(fn);
    if(it != _files.end())
        return it->second;

    if(!lazyLoad || !_loader)
        return NULL;

    // Lazy-load file if it's not in the tree yet
    std::string fn2 = joinPath(fullname(), GetBaseNameFromPath(fn));
    File *f = _loader->Load(fn2.c_str(), fn2.c_str());
    if(f)
        _files[f->name()] = f;
    return f;
}

static void _iterFiles(Files &m, FileEnumCallback f, void *user)
{
    for(Files::iterator it = m.begin(); it != m.end(); ++it)
        f(it->second.content(), user);
}

void Dir::forEachFile(FileEnumCallback f, void *user /* = NULL */, bool safe /* = false */)
{
    load();
    if(safe)
    {
        Files cp = _files;
        _iterFiles(cp, f, user);
    }
    else
        _iterFiles(_files, f, user);
}


bool Dir::add(File *f)
{
    if(!f)
        return false;

    Files::iterator it = _files.find(f->name());

    if(it != _files.end())
    {
        File *oldf = it->second;
        if(oldf == f)
            return false;

        _files.erase(it);
    }

    _files[f->name()] = f;
    return true;
}

bool Dir::addRecursive(File *f, size_t skip /* = 0 */)
{
    if(!f)
        return false;
    
    Dir *vdir = this;
    if(f->fullnameLen() - f->nameLen() > skip)
    {
        // figure out directory from full file name
        size_t prefixLen = f->fullnameLen() - f->nameLen() - skip;

        // prefixLen == 0 is invalid, prefixLen == 1 is just a single '/', which will be stripped away below.
        // in both cases, just use 'this'.
        if(prefixLen > 1)
        {
            char *dirname = (char*)VFS_STACK_ALLOC(prefixLen);
            --prefixLen; // -1 to strip the trailing '/'. That's the position where to put the terminating null byte.
            memcpy(dirname, f->fullname() + skip, prefixLen); // copy trailing null byte
            dirname[prefixLen] = 0;
            vdir = safecastNonNull<Dir*>(getDir(dirname, true));
            VFS_STACK_FREE(dirname);
        }
    }

    return vdir->add(f);
}

void Dir::clearGarbage()
{
    DirBase::clearGarbage();
    for(Files::iterator it = _files.begin(); it != _files.end(); ++it)
        it->second->clearGarbage();
}

bool Dir::_addToView(char *path, DirView& view)
{
    if(Dir *dir = safecast<Dir*>(getDir(path)))
    {
        view.add(dir);
        return true;
    }
    return false;
}

DirBase *Dir::getDirByName(const char *dn, bool lazyLoad /* = true */, bool useSubtrees /* = true */)
{
    DirBase *sub;
    if((sub = DirBase::getDirByName(dn, lazyLoad, useSubtrees)))
        return sub;

    if(!lazyLoad || !_loader)
        return NULL;

    // Lazy-load file if it's not in the tree yet
    // TODO: get rid of alloc
    std::string fn2 = joinPath(fullname(), dn);
    sub = _loader->LoadDir(fn2.c_str(), fn2.c_str());
    if(sub)
        _subdirs[sub->name()] = sub;
    return sub;
}

File *Dir::getFileFromSubdir(const char *subdir, const char *file)
{
    Dir *d = safecast<Dir*>(getDirByName(subdir, true, false)); // useSubtrees is irrelevant here
    return d ? d->getFile(file) : NULL;
}



// ----- DiskDir start here -----


DiskDir::DiskDir(const char *path, VFSLoader *ldr) : Dir(path, ldr)
{
}

DiskDir *DiskDir::createNew(const char *dir) const
{
    return new DiskDir(dir, getLoader());
}

void DiskDir::load()
{
    _files.clear();
    _subdirs.clear();
    // TODO: cache existing files and keep them unless they do no longer exist

    StringList li;
    GetFileList(fullname(), li);
    for(StringList::iterator it = li.begin(); it != li.end(); ++it)
    {
        DiskFile *f = new DiskFile(joinPath(fullname(), it->c_str()).c_str());
        _files[f->name()] = f;
    }

    li.clear();
    GetDirList(fullname(), li, 0);
    for(StringList::iterator it = li.begin(); it != li.end(); ++it)
    {
        // GetDirList() returns relative paths, so need to join
        Dir *d = createNew(joinPath(fullname(), it->c_str()).c_str());
        _subdirs[d->name()] = d;
    }
}

VFS_NAMESPACE_END
