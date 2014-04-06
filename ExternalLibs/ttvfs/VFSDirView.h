#ifndef VFS_DIR_VIEW_H
#define VFS_DIR_VIEW_H

#include <vector>
#include "VFSDir.h"

VFS_NAMESPACE_START

class InternalDir;
class File;

class DirView : public DirBase
{
public:
    DirView();
    ~DirView();
    void init(const char *);
    void add(DirBase *);

    virtual File *getFileByName(const char *fn, bool lazyLoad = true);
    virtual void forEachDir(DirEnumCallback f, void *user = NULL, bool safe = false);
    virtual void forEachFile(FileEnumCallback f, void *user = NULL, bool safe = false);

    virtual const char *getType() const { return "DirView"; }
    virtual DirBase *createNew(const char *dir) const { return NULL; }

    bool _addToView(char *path, DirView& view);

protected:

    typedef std::vector<CountedPtr<DirBase> > ViewList;
    ViewList _view;

};


VFS_NAMESPACE_END

#endif

