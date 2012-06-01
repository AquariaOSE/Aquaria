#ifndef VFSFILE_ZIP_H
#define VFSFILE_ZIP_H

#include "VFSFile.h"
#include "miniz.h"

VFS_NAMESPACE_START

class VFSFileZip : public VFSFile
{
public:
    VFSFileZip(mz_zip_archive *zip);
    virtual ~VFSFileZip();
    virtual bool open(const char *mode = NULL);
    virtual bool isopen(void) const;
    virtual bool iseof(void) const;
    virtual bool close(void);
    virtual bool seek(vfspos pos);
    virtual bool flush(void);
    virtual vfspos getpos(void) const;
    virtual unsigned int read(void *dst, unsigned int bytes);
    virtual unsigned int write(const void *src, unsigned int bytes);
    virtual vfspos size(void);
    virtual const void *getBuf(allocator_func alloc = NULL, delete_func del = NULL);
    virtual void dropBuf(bool del);
    virtual const char *getType(void) const { return "Zip"; }

    inline mz_zip_archive_file_stat *getZipFileStat(void) { return &_zipstat; }
    void _init();

protected:
    unsigned int _pos;
    std::string _mode;
    mz_zip_archive_file_stat _zipstat;
    mz_zip_archive *_zip;
    char *_fixedStr; // for \n fixed string in text mode. cleared when mode is changed
};

VFS_NAMESPACE_END

#endif
