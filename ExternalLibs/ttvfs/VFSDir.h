// VFSDir.h - basic directory interface + classes
// For conditions of distribution and use, see copyright notice in VFS.h

#ifndef VFSDIR_H
#define VFSDIR_H

#include "VFSDefines.h"
#include <map>
#include "VFSSelfRefCounter.h"

VFS_NAMESPACE_START

#ifdef VFS_IGNORE_CASE
#  ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable: 4996)
#  endif

struct ci_less
{
    inline bool operator() (const std::string& a, const std::string& b) const
    {
        return VFS_STRICMP(a.c_str(), b.c_str()) < 0;
    }
};

#  ifdef _MSC_VER
#    pragma warning(pop)
#  endif
#endif


class VFSDir;
class VFSFile;

class VFSDir
{
public:

#ifdef VFS_IGNORE_CASE
    typedef std::map<std::string, VFSDir*, ci_less> Dirs;
    typedef std::map<std::string, VFSFile*, ci_less> Files;
#else
    typedef std::map<std::string, VFSDir*> Dirs;
    typedef std::map<std::string, VFSFile*> Files;
#endif

    VFSDir();
    VFSDir(const char *fullpath);
    virtual ~VFSDir();

    /* Load directory with given path. If dir is NULL, reload previously loaded directory.
       If there is no previously loaded directory, load root. */
    virtual unsigned int load(const char *dir = NULL);
    virtual VFSFile *getFile(const char *fn);
    virtual VFSDir *getDir(const char *subdir, bool forceCreate = false);
    virtual VFSDir *createNew(void) const;
    virtual const char *getType(void) const { return "VFSDir"; }

    bool insert(VFSDir *subdir, bool overwrite = true);
    bool merge(VFSDir *dir, bool overwrite = true);
    bool add(VFSFile *f, bool overwrite = true); // add file directly in this dir
    bool addRecursive(VFSFile *f, bool overwrite = true); // traverse subdir tree to find correct subdir; create if not existing


    inline const char *name() const { VFS_GUARD_OPT(this); return _name; }
    inline const char *fullname() const { VFS_GUARD_OPT(this); return _fullname.c_str(); }

    // iterators are NOT thread-safe! If you need to iterate over things in a multithreaded environment,
    // do the locking yourself! (see below)
    inline Files::iterator       fileIter()          { return _files.begin(); }
    inline Files::iterator       fileIterEnd()       { return _files.end(); }
    inline Dirs::iterator        dirIter()           { return _subdirs.begin(); }
    inline Dirs::iterator        dirIterEnd()        { return _subdirs.end(); }
    inline Files::const_iterator fileIter()    const { return _files.begin(); }
    inline Files::const_iterator fileIterEnd() const { return _files.end(); }
    inline Dirs::const_iterator  dirIter()     const { return _subdirs.begin(); }
    inline Dirs::const_iterator  dirIterEnd()  const { return _subdirs.end(); }

    // std::map<std::string,*> stores for files and subdirs
    Files _files;
    Dirs _subdirs;

    // reference counter, does auto-delete holder when it reaches 0. initially 1.
    SelfRefCounter<VFSDir> ref;

    // the following functions should be used before and after an iteration finishes
    // alternatively, VFS_GUARD(dir) can be used to create a locking guard on the stack.
    inline void lock() { _mtx.Lock(); }
    inline void unlock() { _mtx.Unlock(); }
    inline Mutex& mutex() const { return _mtx; }

protected:
    void _setFullName(const char *fullname);
    std::string _fullname;
    const char *_name; // must point to an address constant during object lifetime (like _fullname.c_str() + N)
                       // (not necessary to have an additional string copy here, just wastes memory)
    mutable Mutex _mtx;
};

typedef VFSDir::Files::iterator FileIter;
typedef VFSDir::Dirs::iterator DirIter;
typedef VFSDir::Files::const_iterator ConstFileIter;
typedef VFSDir::Dirs::const_iterator ConstDirIter;

class VFSDirReal : public VFSDir
{
public:
    VFSDirReal();
    virtual ~VFSDirReal() {};
    virtual unsigned int load(const char *dir = NULL);
    virtual VFSDir *createNew(void) const;
    virtual const char *getType(void) const { return "VFSDirReal"; }
};

VFS_NAMESPACE_END

#endif
