// VFSHelper.h - glues it all together and makes use simple
// For conditions of distribution and use, see copyright notice in VFS.h

#ifndef VFSHELPER_H
#define VFSHELPER_H

#include <vector>
#include <list>
#include <string>
#include <iostream>

#include "VFSAtomic.h"


VFS_NAMESPACE_START

class VFSDir;
class VFSDirReal;
class VFSFile;
class VFSLoader;
class VFSArchiveLoader;


/** VFSHelper - extensible class to simplify working with the VFS tree */
class VFSHelper
{
public:
    VFSHelper();
    virtual ~VFSHelper();

    /** Creates the working tree. Required before any files or directories can be accessed.
        Internally, it merges all individual VFS trees into one. If clear is true (default),
        an existing merged tree is dropped, and old and previously added files removed.
        (This is the recommended setting.)
        Mount points and loaders are kept.*/
    virtual void Prepare(bool clear = true);

    /** Re-merges any files in the tree, and optionally reloads files on disk.
        This is useful if files on disk were created or removed, and the tree needs to reflect these changes.
        Calls Prepare(clear) internally. */
    virtual void Reload(bool fromDisk = false, bool clear = false, bool clearMountPoints = false);

    /** Reset an instance to its initial state.
        Drops all archives, loaders, archive loaders, mount points, internal trees, ...*/
    virtual void Clear(void);

    /** Do cleanups from time to time. In base VFSHelper, this is a no-op.
        Extensions may wish to override this method do do cleanup jobs. */
    virtual void ClearGarbage(void);

    /** Load all files from working directory (into an internal tree) */
    bool LoadFileSysRoot(bool recursive);

    /** Mount a directory in the tree to a different location. Requires a previous call to Prepare().
        This can be imagined like a symlink pointing to a different location.
        Be careful not to create circles, this might technically work,
        but confuses the reference counting, causing memory leaks. */
    bool Mount(const char *src, const char *dest, bool overwrite = true);

    /** Drops a directory from the tree. Internally, this calls Reload(false), 
        which is a heavy operation compared to Mount(). Be warned. */
    bool Unmount(const char *src, const char *dest);

    /** Merges a path into the tree. Requires a previous call to Prepare().
        By default the directory is added into the root directory of the merged tree.
        Pass NULL to add the directory to its original location,
        or any other path to add it to that explicit location.
        If loadRec is true, load all subdirs recursively.
        It is advised not to use this to re-add parts already in the tree; use Mount() instead.
        Rule of thumb: If you called LoadFileSysRoot(), do not use this for subdirs.
        Note: Directories mounted with this will return `where` as their full path if it was set.
              Use GetMountPoint() to retrieve the underlying VFSDir object. */
    bool MountExternalPath(const char *path, const char *where = "", bool loadRec = false, bool overwrite = true);

    /** Adds a VFSDir object into the merged tree. If subdir is NULL (the default),
        add into the subdir stored in the VFSDir object. The tree will be extended if target dir does not exist.
        If overwrite is true (the default), files in the tree will be replaced if already existing.
        Requires a previous call to Prepare().
        Like with Mount(); be careful not to create cycles. */
    bool AddVFSDir(VFSDir *dir, const char *subdir = NULL, bool overwrite = true);

    /** Add the contents of an archive file to the tree. By default, the archive can be addressed
        like a folder, e.g. "path/to/example.zip/file.txt".
        Set asSubdir to false to "unpack" the contents of the archive to the containing folder.
        Optionally, the target subdir to mount into can be specified. (See AddVFSDir().)
        Returns a pointer to the actual VFSDir object that represents the added archive, or NULL if failed.
        The opaque pointer is passed directly to each loader and can contain additional parameters,
        such as a password to open the file.
        Read the comments in VFSArchiveLoader.h for an explanation how it works. If you have no idea, leave it NULL,
        because it can easily cause a crash if not used carefully. */
    VFSDir *AddArchive(const char *arch, bool asSubdir = true, const char *subdir = NULL, void *opaque = NULL);

    /** Add a loader that can look for files on demand.
        It is possible (but not a good idea) to add a loader multiple times. */
    void AddLoader(VFSLoader *ldr);

    /** Add an archive loader that can open archives of various types.
        Whenever an archive file is requested to be opened by AddArchive(),
        it is sent through each registered loader until one of them can recognize
        the format and open it. An archive loader stays once registered. */
    void AddArchiveLoader(VFSArchiveLoader *ldr);
    
    /** Get a file from the merged tree. Requires a previous call to Prepare().
        Asks loaders if the file is not in the tree. If found by a loader, the file will be added to the tree.
        The returned pointer is reference counted. In case the file pointer is stored elsewhere,
        do ptr->ref++, and later ptr->ref--. This is to prevent the VFS tree from deleting the file when cleaning up.
        Not necessary if the pointer is just retrieved and used, or temp. stored while the VFS tree is not modified. */
    VFSFile *GetFile(const char *fn);

    /** Get a directory from the merged tree. If create is true and the directory does not exist,
        build the tree structure and return the newly created dir. NULL otherwise.
        Requires a previous call to Prepare().
        Reference counted, same as GetFile(), look there for more info. */
    VFSDir *GetDir(const char* dn, bool create = false);

    /** Returns the tree root, which is usually the working directory. */
    VFSDir *GetDirRoot(void);

    /** Returns one of the root tree sources by their internal name. */
    VFSDir *GetBaseTree(const char *path);

    /** Returns one of the mount points' base directory
        (The one that is normally not acessible) */
    VFSDir *GetMountPoint(const char *path);

    /** Remove a file or directory from the tree */
    //bool Remove(VFSFile *vf);
    //bool Remove(VFSDir *dir);
    //bool Remove(const char *name); // TODO: CODE ME

    inline void lock() { _mtx.Lock(); }
    inline void unlock() { _mtx.Unlock(); }
    inline Mutex& mutex() const { return _mtx; }

    // DEBUG STUFF
    void debugDumpTree(std::ostream& os, VFSDir *start = NULL);

protected:

    /** Drops the merged tree and allows fully re-creating it.
        Overload to do additional cleanup if required. Invoked by Clear() and Prepare(true). */
    virtual void _cleanup(void);

    struct VDirEntry
    {
        VDirEntry() : vdir(NULL), overwrite(false) {}
        VDirEntry(VFSDir *v, std::string mp, bool ow) : vdir(v), mountPoint(mp), overwrite(ow) {}
        VFSDir *vdir;
        std::string mountPoint;
        bool overwrite;
    };

    struct BaseTreeEntry
    {
        std::string source;
        VFSDir *dir;
    };

    typedef std::list<VDirEntry> VFSMountList;
    typedef std::vector<VFSLoader*> LoaderArray;
    typedef std::vector<VFSArchiveLoader*> ArchiveLoaderArray;
    typedef std::vector<BaseTreeEntry> DirArray;


    void _StoreMountPoint(const VDirEntry& ve);
    bool _RemoveMountPoint(const VDirEntry& ve);
    void _ClearMountPoints(void);

    // the VFSDirs are merged in their declaration order.
    // when merging, files already contained can be overwritten by files merged in later.
    VFSDirReal *filesysRoot; // local files on disk (root dir)

    // VFSDirs from various sources are stored here, and will be merged into one final tree
    // by Prepare().
    DirArray trees;

    // If files are not in the tree, maybe one of these is able to find it.
    LoaderArray loaders;

    VFSDir *merged; // contains the merged virtual/actual file system tree

    mutable Mutex _mtx;

private:
    VFSMountList vlist; // all other trees added later, together with path to mount to
    ArchiveLoaderArray archLdrs;
};

VFS_NAMESPACE_END

#endif
