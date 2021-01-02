#include "VFSDirInternal.h"
#include "VFSDirView.h"
#include "VFSInternal.h"
#include "VFSFile.h"
#include "VFSTools.h"
#include <algorithm>

//#include <stdio.h>

VFS_NAMESPACE_START


// Internal class, not to be used outside

InternalDir::InternalDir(const char *fullpath)
: DirBase(fullpath)
{
}

InternalDir::~InternalDir()
{
}

void InternalDir::_clearDirs()
{
    _subdirs.clear();
}

void InternalDir::_clearMounts()
{
    _mountedDirs.clear();
}

InternalDir *InternalDir::createNew(const char *dir) const
{
    return new InternalDir(dir);
}

void InternalDir::close()
{
    for(MountedDirs::iterator it = _mountedDirs.begin(); it != _mountedDirs.end(); ++it)
        (*it)->close();
}

void InternalDir::_addMountDir(CountedPtr<DirBase> d)
{
    if(d.content() == this)
        return;

    // move to end of vector if already mounted
    for(MountedDirs::iterator it = _mountedDirs.begin(); it != _mountedDirs.end(); ++it)
        if(*it == d)
        {
            _mountedDirs.erase(it);
            break;
        }

    _mountedDirs.push_back(d);
}

void InternalDir::_removeMountDir(DirBase *d)
{
    for(MountedDirs::iterator it = _mountedDirs.begin(); it != _mountedDirs.end(); ++it)
        if(it->content() == d)
        {
            _mountedDirs.erase(it);
            return; // pointers are unique
        }
}

File *InternalDir::getFileByName(const char *fn, bool lazyLoad /* = true */)
{
    if(_mountedDirs.size())
        for(MountedDirs::reverse_iterator it = _mountedDirs.rbegin(); it != _mountedDirs.rend(); ++it)
            if(File *f = (*it)->getFileByName(fn, lazyLoad))
                return f;
    return NULL;
}

DirBase *InternalDir::getDirByName(const char *dn, bool lazyLoad /* = true */, bool useSubtrees /* = true */)
{
    DirBase *sub;
    if((sub = DirBase::getDirByName(dn, lazyLoad)))
        return sub;

    if(useSubtrees)
        for(MountedDirs::reverse_iterator it = _mountedDirs.rbegin(); it != _mountedDirs.rend(); ++it)
            if((sub = (*it)->getDirByName(dn, lazyLoad)))
                return sub;

    return NULL;
}

static void _addFileCallback(File *f, void *p)
{
    ((Files*)p)->insert(std::make_pair(f->name(), f)); // only inserts if not exist
}

void InternalDir::forEachFile(FileEnumCallback f, void *user /* = NULL */, bool /*ignored*/)
{
    Files flist; // TODO: optimize allocation
    for(MountedDirs::reverse_iterator it = _mountedDirs.rbegin(); it != _mountedDirs.rend(); ++it)
        (*it)->forEachFile(_addFileCallback, &flist);

    for(Files::iterator it = flist.begin(); it != flist.end(); ++it)
        f(it->second, user);
}

void InternalDir::forEachDir(DirEnumCallback f, void *user /* = NULL */, bool safe /* = false */)
{
    for(MountedDirs::reverse_iterator it = _mountedDirs.rbegin(); it != _mountedDirs.rend(); ++it)
        (*it)->forEachDir(f, user, safe);
}


bool InternalDir::fillView(const char *path, DirView& view)
{
    view.init(path);
    size_t len = view.fullnameLen() + 1;
    char *pathcopy = (char*)VFS_STACK_ALLOC(len);
    memcpy(pathcopy, view.fullname(), len);
    bool added = _addToView(pathcopy, view);
    VFS_STACK_FREE(pathcopy);
    return added;
}

bool InternalDir::_addToView(char *path, DirView& view)
{
    bool added = false;
    SkipSelfPath(path);

    if(!*path)
    {
        for(MountedDirs::iterator it = _mountedDirs.begin(); it != _mountedDirs.end(); ++it)
        {
            added = true;
            view.add(it->content());
        }
    }
    else
    {
        char dummy = 0;
        char slash[2] = {'/', 0};
        char *slashpos = strchr(path, '/');
        char *tail = slashpos ? slashpos+1 : &dummy;
        // if the first char is a slash, use "/" to lookup
        if(slashpos == path)
            path = &slash[0];

        if(slashpos)
            *slashpos = 0;

        //printf("InternalDir::_addToView [%s] [%s]\n", path, tail);


        for(MountedDirs::iterator it = _mountedDirs.begin(); it != _mountedDirs.end(); ++it)
            if(DirBase *subdir = (*it)->getDirByName(path))
                added = subdir->_addToView(tail, view) || added;

        if(InternalDir *subdir = safecast<InternalDir*>(getDirByName(path, true, false)))
            added = subdir->_addToView(tail, view) || added;

        if(slashpos)
            *slashpos = '/';
    }

    return added;
}

File *InternalDir::getFileFromSubdir(const char *subdir, const char *file)
{
    for(MountedDirs::reverse_iterator it = _mountedDirs.rbegin(); it != _mountedDirs.rend(); ++it)
        if(File* f = (*it)->getFileFromSubdir(subdir, file))
            return f;

    InternalDir *d = safecast<InternalDir*>(DirBase::getDirByName(subdir, false, false)); // vcall not required here
    return d ? d->getFile(file) : NULL;
}


VFS_NAMESPACE_END

