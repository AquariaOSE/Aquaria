// VFSFile.cpp - basic file interface + classes
// For conditions of distribution and use, see copyright notice in VFS.h

#include "VFSInternal.h"
#include <stdio.h> // for SEEK_* constants

#include "VFSFile.h"
#include "VFSTools.h"
#include "VFSFileFuncs.h"


VFS_NAMESPACE_START

File::File(const char *name)
{
    _setName(name);
}

File::~File()
{
}

DiskFile::DiskFile(const char *name /* = NULL */)
: File(name), _fh(NULL)
{
}

DiskFile::~DiskFile()
{
    close();
}

bool DiskFile::open(const char *mode /* = NULL */)
{
    if(isopen())
        close();

    _fh = real_fopen(fullname(), mode ? mode : "rb");

    return !!_fh;
}

bool DiskFile::isopen() const
{
    return !!_fh;
}

bool DiskFile::iseof() const
{
    return !_fh || real_feof((FILE*)_fh);
}

void DiskFile::close()
{
    if(_fh)
    {
        real_fclose((FILE*)_fh);
        _fh = NULL;
    }
}

bool DiskFile::seek(vfspos pos, int whence)
{
    return _fh && real_fseek((FILE*)_fh, pos, whence) == 0;
}


bool DiskFile::flush()
{
    return _fh && real_fflush((FILE*)_fh) == 0;
}

vfspos DiskFile::getpos() const
{
    return _fh ? real_ftell((FILE*)_fh) : npos;
}

size_t DiskFile::read(void *dst, size_t bytes)
{
    return _fh ? real_fread(dst, 1, bytes, (FILE*)_fh) : 0;
}

size_t DiskFile::write(const void *src, size_t bytes)
{
    return _fh ? real_fwrite(src, 1, bytes, (FILE*)_fh) : 0;
}

vfspos DiskFile::size()
{
    vfspos sz = 0;
    bool ok = GetFileSize(fullname(), sz);
    return ok ? sz : npos;
}

// ------------- MemFile -----------------------

MemFile::MemFile(const char *name, void *buf, unsigned int size, delete_func delfunc /* = NULL */, DeleteMode delmode /* = ON_CLOSE */)
: File(name), _buf(buf), _pos(0), _size(size), _delfunc(delfunc), _delmode(delmode)
{
}

MemFile::~MemFile()
{
    if(_delmode == ON_DESTROY)
        _clearMem();
}

void MemFile::_clearMem()
{
    if(_delfunc)
        _delfunc(_buf);
    _delfunc = NULL;
    _buf = NULL;
    _size = 0;
    _pos = 0;
}

void MemFile::close()
{
    if(_delmode == ON_CLOSE)
        _clearMem();
}

bool MemFile::seek(vfspos pos, int whence)
{
    switch(whence)
    {
        case SEEK_SET:
            if(pos < _size)
            {
                _pos = pos;
                return true;
            }
            break;

        case SEEK_CUR:
            if(_pos + pos < _size)
            {
                _pos += pos;
                return true;
            }
            break;

        case SEEK_END:
            if(pos < _size)
            {
                _pos = _size - pos;
                return true;
            }
    }
    return false;
}

size_t MemFile::read(void *dst, size_t bytes)
{
    if(iseof())
        return 0;
    size_t rem = std::min((size_t)(_size - _pos), bytes);

    memcpy(dst, (char*)_buf + _pos, rem);
    return rem;
}

size_t MemFile::write(const void *src, size_t bytes)
{
    if(iseof())
        return 0;
    size_t rem = std::min((size_t)(_size - _pos), bytes);

    memcpy((char*)_buf + _pos, src, rem);
    return rem;
}

VFS_NAMESPACE_END
