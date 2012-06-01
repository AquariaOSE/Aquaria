// VFSDir.h - basic directory interface + classes
// For conditions of distribution and use, see copyright notice in VFS.h

#ifndef VFSDIR_H
#define VFSDIR_H

#include "VFSBase.h"
#include <map>
#include <cstring>

#ifdef VFS_USE_HASHMAP
#  include "VFSHashmap.h"
#  include "VFSTools.h"
#endif

VFS_NAMESPACE_START


#ifdef VFS_IGNORE_CASE
#  ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable: 4996)
#  endif

struct ci_less
{
    inline bool operator() (const char *a, const char *b) const
    {
        return VFS_STRICMP(a, b) < 0;
    }
};

struct ci_equal
{
    inline bool operator() (const char *a, const char *b) const
    {
        return !VFS_STRICMP(a, b);
    }
};

inline int casecmp(const char *a, const char *b)
{
    return VFS_STRICMP(a, b);
}

#  ifdef _MSC_VER
#    pragma warning(pop)
#  endif

#else // VFS_IGNORE_CASE

struct cs_less
{
    inline bool operator() (const char *a, const char *b) const
    {
        return strcmp(a, b) < 0;
    }
};

inline int casecmp(const char *a, const char *b)
{
    return strcmp(a, b);
}


#endif // VFS_IGNORE_CASE


#ifdef VFS_USE_HASHMAP

struct hashmap_eq
{
    inline bool operator() (const char *a, const char *b, size_t h, const VFSBase *itm) const
    {
        // quick check - instead of just comparing the strings,
        // check the hashes first. If they don't match there is no
        // need to check the strings at all.
        return itm->hash() == h && !casecmp(a, b);
    }
};

struct charptr_hash
{
    inline size_t operator()(const char *s)
    {
        // case sensitive or in-sensitive, depending on config
        return STRINGHASH(s);
    }
};

#endif // VFS_USE_HASHMAP


class VFSDir;
class VFSFile;

typedef void (*FileEnumCallback)(VFSFile *vf, void *user);
typedef void (*DirEnumCallback)(VFSDir *vd, void *user);


class VFSDir : public VFSBase
{
public:

    // bitmask
    enum EntryFlags
    {
        NONE = 0,
        MOUNTED = 1
    };

    template<typename T> struct MapEntry
    {
        MapEntry() {}
        MapEntry(T *p, EntryFlags flg = NONE) : ptr(p), flags(flg) {}
        inline bool isMounted() { return flags & MOUNTED; }

        T *ptr;
        EntryFlags flags;
    };

    // Avoid using std::string as key.
    // The file names are known to remain constant during each object's lifetime,
    // so just keep the pointers and use an appropriate comparator function.
#ifdef VFS_USE_HASHMAP
        // VFS_IGNORE_CASE already handled in hash generation
        typedef HashMap<const char *, MapEntry<VFSDir>, charptr_hash, hashmap_eq> Dirs;
        typedef HashMap<const char *, MapEntry<VFSFile>, charptr_hash, hashmap_eq> Files;
#else
#  ifdef VFS_IGNORE_CASE
        typedef std::map<const char *, MapEntry<VFSDir>, ci_less> Dirs;
        typedef std::map<const char *, MapEntry<VFSFile>, ci_less> Files;
#  else
        typedef std::map<const char *, MapEntry<VFSDir>, cs_less> Dirs;
        typedef std::map<const char *, MapEntry<VFSFile>, cs_less> Files;
#  endif
#endif

    VFSDir(const char *fullpath);
    virtual ~VFSDir();

    /** Enumerate directory with given path. Keeps previously loaded entries.
        Returns the amount of files found. */
    virtual unsigned int load(bool recursive);

    /** Creates a new virtual directory of an internally specified type. */
    virtual VFSDir *createNew(const char *dir) const;

    /** For debugging. Does never return NULL. */
    virtual const char *getType() const { return "VFSDir"; }

    /** Can be overloaded if necessary. Called by VFSHelper::ClearGarbage() */
    virtual void clearGarbage() {}

    /** Can be overloaded to close resources this dir keeps open */
    virtual bool close() { return true; }

    /** Returns a file for this dir's subtree. Descends if necessary.
    Returns NULL if the file is not found. */
    VFSFile *getFile(const char *fn);

    /** Returns a subdir, descends if necessary. If forceCreate is true,
    create directory tree if it does not exist, and return the originally requested
    subdir. Otherwise return NULL if not found. */
    VFSDir *getDir(const char *subdir, bool forceCreate = false);

    /** Recursively drops all files/dirs that were mounted into this directory (and subdirs) */
    void clearMounted();

    /** Iterate over all files or directories, calling a callback function,
        optionally with additional userdata. If safe is true, iterate over a copy.
        This is useful if the callback function modifies the tree, e.g.
        adds or removes files. */
    void forEachFile(FileEnumCallback f, void *user = NULL, bool safe = false);
    void forEachDir(DirEnumCallback f, void *user = NULL, bool safe = false);


    /* Below is for internal use -- take care if using these externally! */
    bool insert(VFSDir *subdir, bool overwrite, EntryFlags flag);
    bool merge(VFSDir *dir, bool overwrite, EntryFlags flag);

    /** Adds a file directly to this directory, allows any name.
    If another file with this name already exists, optionally drop the old one out.
    Returns whether the file was actually added. */
    bool add(VFSFile *f, bool overwrite, EntryFlags flag);

    /** Like add(), but if the file name contains a path, descend the tree to the target dir.
        Not-existing subdirs are created on the way. */
    bool addRecursive(VFSFile *f, bool overwrite, EntryFlags flag);

protected:

    // std::map<const char*,X> or ttvfs::HashMap<const char*, X> stores for files and subdirs.
    Files _files;
    Dirs _subdirs;
};

typedef VFSDir::Files::iterator FileIter;
typedef VFSDir::Dirs::iterator DirIter;

class VFSDirReal : public VFSDir
{
public:
    VFSDirReal(const char *dir);
    virtual ~VFSDirReal() {};
    virtual unsigned int load(bool recursive);
    virtual VFSDir *createNew(const char *dir) const;
    virtual const char *getType(void) const { return "VFSDirReal"; }
};

VFS_NAMESPACE_END

#endif
