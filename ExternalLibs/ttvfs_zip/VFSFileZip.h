#ifndef VFSFILE_ZIP_H
#define VFSFILE_ZIP_H

#include "VFSFile.h"
#include "VFSZipArchiveRef.h"

VFS_NAMESPACE_START

class ZipFile : public File
{
public:
    ZipFile(const char *name, ZipArchiveRef *zref, vfspos uncompSize, unsigned int fileIdx);
    virtual ~ZipFile();
    virtual bool open(const char *mode = NULL);
    virtual bool isopen() const;
    virtual bool iseof() const;
    virtual void close();
    virtual bool seek(vfspos pos, int whence);
    virtual bool flush();
    virtual vfspos getpos() const;
    virtual unsigned int read(void *dst, unsigned int bytes);
    virtual unsigned int write(const void *src, unsigned int bytes);
    virtual vfspos size();
    virtual const char *getType() const { return "ZipFile"; }

protected:
    bool unpack();

    char *_buf;
    vfspos _pos;
    CountedPtr<ZipArchiveRef> _archiveHandle;
    vfspos _uncompSize;
    unsigned int _fileIdx;
    std::string _mode;
};

VFS_NAMESPACE_END

#endif
