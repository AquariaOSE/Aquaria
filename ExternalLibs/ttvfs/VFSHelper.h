// VFSHelper.h - glues it all together and makes use simple
// For conditions of distribution and use, see copyright notice in VFS.h

#ifndef VFSHELPER_H
#define VFSHELPER_H

#include <set>
#include <vector>
#include <deque>
#include <list>

#include "VFSAtomic.h"

VFS_NAMESPACE_START

class VFSDir;
class VFSDirReal;
class VFSFile;
class VFSLoader;


/** VFSHelper - extensible class to simplify working with the VFS tree
  * Contains a set of useful functions that should be useful for anyone.
  * This class may be overridden to support adding any source in a comfortable way.
  * 
  * Note: This class uses VFS_LAST_HELPER_CLASS, which should always store the last
  * class derived from VFSHelper. This is supposed to make it easy to make extensions like this:

#include "VFSHelperExtra.h" // defines a VFSHelperExtra class that is somehow derived from VFSHelper
                            // and follows the same rules as explained below.

class VFSHelperArchive : public VFS_LAST_HELPER_CLASS
{
private:
    typedef VFS_LAST_HELPER_CLASS super;
public:
    // .... class members ....
};

#undef VFS_LAST_HELPER_CLASS
#define VFS_LAST_HELPER_CLASS VFSHelperArchive


  * Used this way, only the order in which VFSHelper extension classes are included matters.
  * No code changes are required to get a nice inheritance and priority chain working.
  *
*/

#ifdef VFS_LAST_HELPER_CLASS
#  error VFS_LAST_HELPER_CLASS defined before VFSHelper class decl, check your include order!
#endif

class VFSHelper
{
public:
    VFSHelper();
    virtual ~VFSHelper();

    /** Creates the working tree. Required before any files or directories can be accessed.
        Internally, it merges all individual VFS trees into one. If clear is true (default),
        an existing merged tree is dropped, and old and previously added files and loaders removed.
        (This is the recommended setting.) */
    virtual void Prepare(bool clear = true);

    /** Re-merges any files in the tree, and optionally reloads files on disk.
        This is useful if files on disk were created or removed, and the tree needs to reflect these changes. */
    virtual void Reload(bool fromDisk = false);

    /** Reset an instance to its initial state */
    virtual void Clear(void);

    /** Load all files from working directory (into an internal tree) */
    bool LoadFileSysRoot(void);

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
        It is advised not to use this to re-add parts already in the tree; use Mount() instead.
        Rule of thumb: If you called LoadFileSysRoot(), do not use this for subdirs. */
    bool MountExternalPath(const char *path, const char *where = "");

    /** Adds a VFSDir object into the merged tree. If subdir is NULL (the default),
        add into the subdir stored in the VFSDir object. The tree will be extended if target dir does not exist.
        If overwrite is true (the default), files in the tree will be replaced if already existing.
        Requires a previous call to Prepare().
        Like with Mount(); be careful not to create cycles. */
    bool AddVFSDir(VFSDir *dir, const char *subdir = NULL, bool overwrite = true);

    /** Add a loader that can look for files on demand.
        It will be deleted if Prepare(true) is called.
        It is possible (but not a good idea) to add a loader multiple times. */
    inline void AddLoader(VFSLoader *ldr);
    
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

    /** Remove a file or directory from the tree */
    //bool Remove(VFSFile *vf);
    //bool Remove(VFSDir *dir);
    //bool Remove(const char *name); // TODO: CODE ME

    /** This depends on the class type and stays constant. */
    inline unsigned int FixedLoadersCount(void) const { return (unsigned int)fixedLdrs.size(); }

    inline void lock() { _mtx.Lock(); }
    inline void unlock() { _mtx.Unlock(); }
    inline Mutex& mutex() const { return _mtx; }

    // DEBUG STUFF
    void debugDumpTree(std::ostream& os, VFSDir *start = NULL);

protected:

    /** Drops the merged tree and additional mount points and dynamic loaders.
        Overload to do additional cleanup if required. Invoked by Clear() and Prepare(true). */
    virtual void _cleanup(void);

    /** Add a fixed VFSLoader. Returns its array index in fixedLdrs. */
    unsigned int _AddFixedLoader(VFSLoader *ldr = NULL);

    struct VDirEntry
    {
        VDirEntry() : vdir(NULL), overwrite(false) {}
        VDirEntry(VFSDir *v, std::string mp, bool ow) : vdir(v), mountPoint(mp), overwrite(ow) {}
        VFSDir *vdir;
        std::string mountPoint;
        bool overwrite;
    };

    typedef std::list<VDirEntry> VFSMountList;
    typedef std::vector<VFSLoader*> LoaderArray;
    typedef std::deque<VFSLoader*> LoaderList;
    typedef std::vector<VFSDir*> DirArray;


    void _StoreMountPoint(const VDirEntry& ve);

    bool _RemoveMountPoint(const VDirEntry& ve);

    // the VFSDirs are merged in their declaration order.
    // when merging, files already contained can be overwritten by files merged in later.
    VFSDirReal *filesysRoot; // local files on disk (root dir)

    // Additional tree stores, to be filled by subclasses if needed.
    DirArray preRoot; // VFSDirs in here will be merged in, before the actual disk files.
                      // Means files on disk will overwrite existing entries.
    DirArray postRoot; // Will be merged after the disk files, and overwrite prev. merged files.
    // Both may contain NULLs.

    // if files are not in the tree, maybe one of these is able to find it. May contain NULLs.
    LoaderArray fixedLdrs; // defined by class type, stays constant during object lifetime
    LoaderList dynLdrs; // dynamically added on demand, deleted on _cleanup()

    VFSDir *merged; // contains the merged virtual/actual file system tree

    mutable Mutex _mtx;

private:
    unsigned int _ldrDiskId;
    VFSMountList vlist; // all other dirs added later, together with path to mount to
};

#undef VFS_LAST_HELPER_CLASS
#define VFS_LAST_HELPER_CLASS VFSHelper

VFS_NAMESPACE_END

#endif
