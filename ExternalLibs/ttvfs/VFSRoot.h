// VFSRoot.h - glues it all together and makes use simple
// For conditions of distribution and use, see copyright notice in VFS.h

#ifndef VFSHELPER_H
#define VFSHELPER_H

#include <vector>
#include <list>
#include <string>
#include <iosfwd>

#include "VFSRefcounted.h"


VFS_NAMESPACE_START

class DirBase;
class Dir;
class InternalDir;
class File;
class VFSLoader;
class VFSArchiveLoader;
class DirView;


/** Root - extensible class to simplify working with the VFS tree */
class Root
{
public:
    Root();
    virtual ~Root();

    /** Reset an instance to its initial state.
        Drops all archives, loaders, archive loaders, mount points, internal trees, ...*/
    virtual void Clear();

    /** Do cleanups from time to time. For internal classes, this is a no-op.
        Extensions may wish to override this method do do cleanup jobs. */
    virtual void ClearGarbage();

    /** Mount a directory in the tree to a different location. Requires a previous call to Prepare().
        This can be imagined like the contents of a directory appearing in a different location.
        Be careful not to create circles! */
    void Mount(const char *src, const char *dest);

    /** Drops a directory from the tree. Internally, this calls Reload(false),
        which is a heavy operation compared to Mount(). Be warned. */
    bool Unmount(const char *src, const char *dest);

    /** Adds a Dir object into the merged tree. If subdir is NULL (the default),
        add into the subdir stored in the Dir object. The tree will be extended if target dir does not exist.
        Files in the tree will be replaced if already existing.
        Like with Mount(); be careful not to create cycles. */
    void AddVFSDir(DirBase *dir, const char *subdir = NULL);

    /** Add an archive file to the tree, which can then be addressed like a folder,
        e.g. "path/to/example.zip/file.txt".
        Returns a pointer to the actual Dir object that represents the added archive, or NULL if failed.
        The opaque pointer is passed directly to each loader and can contain additional parameters,
        such as a password to open the file.
        Read the comments in VFSArchiveLoader.h for an explanation how it works.
        If you have no idea, leave it NULL, because it can easily cause a crash if not used carefully. */
    Dir *AddArchive(const char *arch, void *opaque = NULL);
    Dir *AddArchive(File *file, const char *path = NULL, void *opaque = NULL);

    /** Add a loader that can look for files on demand.
        Do not add more then once instance of a loader type. */
    void AddLoader(VFSLoader *ldr, const char *path = NULL);

    /** Add an archive loader that can open archives of various types.
        Whenever an archive file is requested to be opened by AddArchive(),
        it is sent through each registered loader until one of them can recognize
        the format and open it. */
    void AddArchiveLoader(VFSArchiveLoader *ldr);

    /** Get a file from the merged tree. Asks loaders if the file is not in the tree.
        If found by a loader, the file will be added to the tree. */
    File *GetFile(const char *fn);

    /** Fills a DirView object with a list of directories that match the specified path.
        This is the preferred way to enumerate directories, as it respects and collects
        mount points during traversal. The DirView instance can be re-used until an (un-)mounting
        operation takes place. If the content of directories changes, this is reflected in the view.
        (Added dirs or files will appear, removed ones disappear).
        Use DirView::forEachFile() or DirView::forEachDir() to iterate. */
    bool FillDirView(const char *path, DirView& view);

    /** Remove a file or directory from the tree */
    //bool Remove(File *vf);
    //bool Remove(Dir *dir);
    //bool Remove(const char *name); // TODO: CODE ME


    /** Returns the tree root, which is usually the working directory.
    Same as GetDir("").
    You will most likely not need this function. */
    DirBase *GetDirRoot();

    /** Get a directory from the merged tree. If create is true and the directory does not exist,
    build the tree structure and return the newly created dir. NULL otherwise.
    You will most likely not need this function.
    Use FillDirView() on a DirView object to iterate over directory contents. */
    DirBase *GetDir(const char* dn, bool create = false);

    // DEBUG STUFF
    void debugDumpTree(std::ostream& os, const char *path, int level);

protected:

    InternalDir *_GetDirByLoader(VFSLoader *ldr, const char *fn, const char *unmangled);

    typedef std::vector<CountedPtr<VFSLoader> > LoaderArray;
    typedef std::vector<CountedPtr<VFSArchiveLoader> > ArchiveLoaderArray;

    // If files are not in the tree, maybe one of these is able to find it.
    LoaderArray loaders;

    CountedPtr<InternalDir> merged; // contains the merged virtual/actual file system tree

    ArchiveLoaderArray archLdrs;
};

VFS_NAMESPACE_END

#endif
