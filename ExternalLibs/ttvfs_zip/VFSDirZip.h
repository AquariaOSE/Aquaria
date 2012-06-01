#ifndef VFSDIR_ZIP_H
#define VFSDIR_ZIP_H

#include "VFSDir.h"

#include "miniz.h"

VFS_NAMESPACE_START

class VFSFile;

class VFSDirZip : public VFSDir
{
public:
    VFSDirZip(VFSFile *zf);
    virtual ~VFSDirZip();
    virtual unsigned int load(bool recusive);
    virtual VFSDir *createNew(const char *dir) const;
    virtual const char *getType() const { return "VFSDirZip"; }
    virtual bool close();

    inline mz_zip_archive *getZip() { return &_zip; }

protected:
    VFSFile *_zf;
    mz_zip_archive _zip;
    std::string zipfilename;
};

VFS_NAMESPACE_END

#endif
