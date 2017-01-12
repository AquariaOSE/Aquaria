#include "VFSFileZip.h"
#include "VFSInternal.h"
#include "VFSTools.h"
#include "VFSDir.h"
#include <stdio.h>
#include "miniz.h"

VFS_NAMESPACE_START


ZipFile::ZipFile(const char *name, ZipArchiveRef *zref, unsigned int fileIdx)
: File(joinPath(zref->fullname(), name).c_str())
, _buf(NULL)
, _pos(0)
, _archiveHandle(zref)
, _bufSize(0)
, _fileIdx(fileIdx)
, _mode("rb") // binary mode by default
{
}

ZipFile::~ZipFile()
{
    close();
}


bool ZipFile::open(const char *mode /* = NULL */)
{
    _pos = 0;
    if(!mode)
        mode = "rb";
    else if(strchr(mode, 'w') || strchr(mode, 'a'))
        return false; // writing not yet supported
    if(_mode != mode)
    {
        delete [] _buf;
        _buf = NULL;
        _mode = mode;
    }
    return true; // does not have to be opened
}

bool ZipFile::isopen() const
{
    return true; // is always open
}

bool ZipFile::iseof() const
{
    return _pos >= _bufSize;
}

void ZipFile::close()
{
    //flush(); // TODO: write to zip file on close

    delete [] _buf;
    _buf = NULL;
    _bufSize = 0;
}

bool ZipFile::seek(vfspos pos, int whence)
{
    const vfspos end = 0xFFFFFFFF;
    switch(whence)
    {
        case SEEK_SET:
            if(pos >= end) // zip files have uint32 range only
                return false;
            _pos = pos;
            break;

        case SEEK_CUR:
            if(_pos + pos >= end)
                return false;
            _pos += pos;
            break;

        case SEEK_END:
            if(pos >= _bufSize)
                return false;
            _pos = _bufSize - pos;
            break;

        default:
            return false;
    }

    return true;
}

bool ZipFile::flush()
{
    // FIXME: use this to actually write to zip file?
    return false;
}

vfspos ZipFile::getpos() const
{
    return _pos;
}

size_t ZipFile::read(void *dst, size_t bytes)
{
    if(!_buf && !unpack())
        return 0;

    char *startptr = _buf + _pos;
    char *endptr = _buf + size();
    bytes = std::min<size_t>(endptr - startptr, bytes); // limit in case reading over buffer size
    memcpy(dst, startptr, bytes); //  binary copy
    _pos += bytes;
    return bytes;
}

size_t ZipFile::write(const void *src, size_t bytes)
{
    // TODO: implement actually writing to the underlying Zip file.
    //printf("NYI: ZipFile::write()");

    return 0;
}

#define MZ ((mz_zip_archive*)_archiveHandle->mz)

vfspos ZipFile::size()
{
    if(_buf && _bufSize)
        return _bufSize;

    if(!_archiveHandle->openRead())
        return npos;

    // FIXME: this is not 100% safe. The file index may change if the zip file is changed externally while closed
    mz_zip_archive_file_stat zstat;
    if(!mz_zip_reader_file_stat(MZ, _fileIdx, &zstat))
        return npos;

    return (vfspos)zstat.m_uncomp_size;
}

bool ZipFile::unpack()
{
    close(); // delete the buffer

    const vfspos sz = size(); // will reopen the file
    if(sz == npos)
        return false;

    _buf = new char[size_t(sz) + 1];
    if(!_buf)
        return false;

    if(!mz_zip_reader_extract_to_mem(MZ, _fileIdx, _buf, (size_t)sz, 0))
    {
        delete [] _buf;
        _buf = NULL;
        return false; // this should not happen
    }

    _bufSize = sz;

    // In case of text data, make sure the buffer is always terminated with '\0'.
    // Won't hurt for binary data, so just do it in all cases.
    _buf[sz] = 0;
    if(_mode.find("b") == std::string::npos) // text mode?
    {
        _bufSize = (vfspos)strnNLcpy(_buf, _buf);
    }

    return true;
}


VFS_NAMESPACE_END
