// VFSDir.cpp - basic directory interface + classes
// For conditions of distribution and use, see copyright notice in VFS.h

#include "VFSInternal.h"
#include "VFSTools.h"
#include "VFSFile.h"
#include "VFSDir.h"

VFS_NAMESPACE_START

VFSDir::VFSDir()
: ref(this), _name(NULL)
{
}

VFSDir::VFSDir(const char *fullpath)
: ref(this)
{
    _setFullName(fullpath);
}

VFSDir::~VFSDir()
{
    for(Files::iterator it = _files.begin(); it != _files.end(); it++)
        it->second->ref--;
    for(Dirs::iterator it = _subdirs.begin(); it != _subdirs.end(); it++)
        it->second->ref--;
}

void VFSDir::_setFullName(const char *fullname)
{
    _fullname = FixPath(fullname);
    _name = PathToFileName(_fullname.c_str());
}

VFSDir *VFSDir::createNew(void) const
{
    return new VFSDir;
}

unsigned int VFSDir::load(const char *dir /* = NULL */)
{
    return 0;
}


bool VFSDir::add(VFSFile *f, bool overwrite /* = true */)
{
    if(!f)
        return false;

    VFS_GUARD_OPT(this);

    Files::iterator it = _files.find(f->name());
    
    if(it != _files.end())
    {
        if(overwrite)
        {
            VFSFile *oldf = it->second;
            if(oldf == f)
                return false;

            oldf->ref--;
            _files.erase(it);
        }
        else
            return false;
    }

    f->ref++;
    _files[f->name()] = f;
    return true;
}

bool VFSDir::addRecursive(VFSFile *f, bool overwrite /* = true */)
{
    if(!f)
        return false;

    VFS_GUARD_OPT(this);

    // figure out directory from full file name
    std::string dirname(f->fullname());
    size_t pathend = dirname.find_last_of("/\\");
    VFSDir *vdir;
    if(pathend != std::string::npos)
    {
        dirname = dirname.substr(0, pathend);
        vdir = getDir(dirname.c_str(), true);
    }
    else
        vdir = this;

    return vdir->add(f, true);
}

bool VFSDir::merge(VFSDir *dir, bool overwrite /* = true */)
{
    if(!dir)
        return false;

    bool result = false;
    VFS_GUARD_OPT(this);

    for(Files::iterator it = dir->_files.begin(); it != dir->_files.end(); it++)
        result = add(it->second, overwrite) || result;

    for(Dirs::iterator it = dir->_subdirs.begin(); it != dir->_subdirs.end(); it++)
        result = insert(it->second, overwrite) || result;
    return result;
}

bool VFSDir::insert(VFSDir *subdir, bool overwrite /* = true */)
{
    if(!subdir)
        return false;

    VFS_GUARD_OPT(this);
    Dirs::iterator it = _subdirs.find(subdir->name());
    VFSDir *mydir;
    if(it != _subdirs.end())
    {
        mydir = it->second;
        //return it->second->merge(subdir, overwrite);
    }
    else
    {
        // create a new subtree, not to pollute the original one with data that may be added later
        mydir = subdir->createNew(); // create subdir of same type
        mydir->_setFullName(subdir->fullname());
        _subdirs[mydir->name()] = mydir;
    }

    return mydir->merge(subdir, overwrite);
}

VFSFile *VFSDir::getFile(const char *fn)
{
    char *slashpos = (char *)strchr(fn, '/');

    // if there is a '/' in the string, descend into subdir and continue there
    if(slashpos)
    {
        const char *sub = slashpos + 1;
        std::string t(fn, slashpos - fn);
        VFS_GUARD_OPT(this);
        VFSDir *subdir = getDir(t.c_str()); // fn is null-terminated early here
        return subdir ? subdir->getFile(sub) : NULL;
    }

    // no subdir? file must be in this dir now.
    VFS_GUARD_OPT(this);
    Files::iterator it = _files.find(fn);
    return it != _files.end() ? it->second : NULL;
}

VFSDir *VFSDir::getDir(const char *subdir, bool forceCreate /* = false */)
{
    if(!subdir[0] || (subdir[0] == '.' && (!subdir[1] || subdir[1] == '/'))) // empty string or "." or "./" ? use this.
        return this;

    VFSDir *ret = NULL;
    char *slashpos = (char *)strchr(subdir, '/');

    // if there is a '/' in the string, descend into subdir and continue there
    if(slashpos)
    {
        const char *sub = slashpos + 1;
        std::string t(subdir, slashpos - subdir);
        VFS_GUARD_OPT(this);
        Dirs::iterator it = _subdirs.find(t);
        if(it != _subdirs.end())
        {
            ret = it->second->getDir(sub, forceCreate); // descend into subdirs
        }
        else if(forceCreate)
        {
            VFSDir *ins = createNew();
            std::string newname(fullname());
            newname += '/';
            newname += t;
            ins->_setFullName(newname.c_str());
            _subdirs[ins->name()] = ins;
            ret = ins->getDir(sub, true); // create remaining structure
        }
    }
    else
    {
        VFS_GUARD_OPT(this);
        Dirs::iterator it = _subdirs.find(subdir);
        if(it != _subdirs.end())
            ret = it->second;
        else if(forceCreate)
        {
            ret = createNew();
            std::string newname(fullname());
            newname += '/';
            newname += subdir;
            ret->_setFullName(newname.c_str());
            _subdirs[ret->name()] = ret;
        }
    }

    return ret;
}



// ----- VFSDirReal start here -----


VFSDirReal::VFSDirReal() : VFSDir()
{
}

VFSDir *VFSDirReal::createNew(void) const
{
    return new VFSDirReal;
}

unsigned int VFSDirReal::load(const char *dir /* = NULL */)
{
    VFS_GUARD_OPT(this);
    if(dir)
        _setFullName(dir);

    StringList li;
    GetFileList(_fullname.c_str(), li);
    for(StringList::iterator it = li.begin(); it != li.end(); it++)
    {
        if(VFSFile *oldf = getFile(it->c_str()))
            oldf->ref--;
        VFSFileReal *f = new VFSFileReal((_fullname + '/' + *it).c_str());
        _files[f->name()] = f;
    }
    unsigned int sum = li.size();

    li.clear();
    GetDirList(_fullname.c_str(), li, false);
    for(std::deque<std::string>::iterator it = li.begin(); it != li.end(); it++)
    {
        if(VFSDir *oldd = getDir(it->c_str()))
            oldd->ref--;
        VFSDir *d = createNew();
        std::string full(_fullname);
        full += '/';
        full += *it;
        sum += d->load(full.c_str()); // GetDirList() always returns relative paths
        _subdirs[d->name()] = d;
    }
    return sum;
}

VFS_NAMESPACE_END
