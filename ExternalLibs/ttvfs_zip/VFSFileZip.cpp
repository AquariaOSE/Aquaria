#include "VFSFileZip.h"
#include "VFSInternal.h"
#include "VFSTools.h"
#include "VFSDir.h"
#include <stdio.h>
#include "miniz.h"

VFS_NAMESPACE_START


ZipFile::ZipFile(const char *name, ZipArchiveRef *zref, vfspos uncompSize, unsigned int fileIdx)
: File(joinPath(zref->fullname(), name).c_str())
, _buf(NULL)
, _pos(0)
, _archiveHandle(zref)
, _uncompSize(uncompSize)
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
    return _pos >= _uncompSize;
}

void ZipFile::close()
{
    //flush(); // TODO: write to zip file on close

    delete []_buf;
    _buf = NULL;
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
            if(pos >= _uncompSize)
                return false;
            _pos = _uncompSize - pos;
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

unsigned int ZipFile::read(void *dst, unsigned int bytes)
{
    if(!_buf && !unpack())
        return 0;

    char *startptr = _buf + _pos;
    char *endptr = _buf + size();
    bytes = std::min((unsigned int)(endptr - startptr), bytes); // limit in case reading over buffer size
    //if(_mode.find('b') == std::string::npos)
    //    strnNLcpy((char*)dst, (const char*)startptr, bytes); // non-binary == text mode
    //else
        memcpy(dst, startptr, bytes); //  binary copy
    _pos += bytes;
    return bytes;
}

unsigned int ZipFile::write(const void *src, unsigned int bytes)
{
    // TODO: implement actually writing to the underlying Zip file.

    return 0;
}

vfspos ZipFile::size()
{
    return (vfspos)_uncompSize;
}

#define MZ ((mz_zip_archive*)_archiveHandle->mz)

bool ZipFile::unpack()
{
    delete [] _buf;

    if(!_archiveHandle->openRead())
        return false; // can happen if the underlying zip file was deleted

    // In case of text data, make sure the buffer is always terminated with '\0'.
    // Won't hurt for binary data, so just do it in all cases.
    size_t sz = (size_t)size();
    _buf = new char[sz + 1];
    if(!_buf)
        return false;

    if(!mz_zip_reader_extract_to_mem(MZ, _fileIdx, _buf, sz, 0))
    {
        delete [] _buf;
        _buf = NULL;
        return false; // this should not happen
    }

    _buf[sz] = 0;
    if(_mode.find("b") == std::string::npos) // text mode?
    {
        _uncompSize = strnNLcpy(_buf, _buf);
    }

    return true;
}


VFS_NAMESPACE_END
