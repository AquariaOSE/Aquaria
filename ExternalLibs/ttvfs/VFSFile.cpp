// VFSFile.cpp - basic file interface + classes
// For conditions of distribution and use, see copyright notice in VFS.h

#include "VFSInternal.h"
#include "VFSFile.h"
#include "VFSTools.h"

VFS_NAMESPACE_START

VFSFile::VFSFile()
: ref(this)
{
}

VFSFileReal::VFSFileReal(const char *name /* = NULL */)
: VFSFile()
{
    _buf = NULL;
    _setName(name);
    _fh = NULL;
    _size = npos;
}

VFSFileReal::~VFSFileReal()
{
    close();
    dropBuf(true);
}

// call this only with a lock held!
void VFSFileReal::_setName(const char *n)
{
    if(n && *n)
    {
        _fullname = FixPath(n);
        _name = PathToFileName(_fullname.c_str());
    }
}

bool VFSFileReal::open(const char *fn /* = NULL */, const char *mode /* = NULL */)
{
    VFS_GUARD_OPT(this);

    if(isopen())
        close();

    dropBuf(true);

    _setName(fn);

    _fh = fopen(_fullname.c_str(), mode ? mode : "rb");
    if(!_fh)
        return false;

    fseek((FILE*)_fh, 0, SEEK_END);
    _size = getpos();
    fseek((FILE*)_fh, 0, SEEK_SET);

    return true;
}

bool VFSFileReal::isopen(void) const
{
    VFS_GUARD_OPT(this);
    return !!_fh;
}

bool VFSFileReal::iseof(void) const
{
    VFS_GUARD_OPT(this);
    return !_fh || feof((FILE*)_fh);
}

const char *VFSFileReal::name(void) const
{
    VFS_GUARD_OPT(this);
    return _name;
}

const char *VFSFileReal::fullname(void) const
{
    VFS_GUARD_OPT(this);
    return _fullname.c_str();
}

bool VFSFileReal::close(void)
{
    VFS_GUARD_OPT(this);
    if(_fh)
    {
        fclose((FILE*)_fh);
        _fh = NULL;
    }
    return true;    
}

bool VFSFileReal::seek(vfspos pos)
{
    VFS_GUARD_OPT(this);
    if(!_fh)
        return false;
    fseek((FILE*)_fh, pos, SEEK_SET);
    return true;
}

bool VFSFileReal::seekRel(vfspos offs)
{
    VFS_GUARD_OPT(this);
    if(!_fh)
        return false;
    fseek((FILE*)_fh, offs, SEEK_CUR);
    return true;
}

bool VFSFileReal::flush(void)
{
    VFS_GUARD_OPT(this);
    if(_fh)
        return false;
    fflush((FILE*)_fh);
    return true;
}

vfspos VFSFileReal::getpos(void) const
{
    VFS_GUARD_OPT(this);
    if(!_fh)
        return npos;
    return ftell((FILE*)_fh);
}

unsigned int VFSFileReal::read(void *dst, unsigned int bytes)
{
    VFS_GUARD_OPT(this);
    if(!_fh)
        return npos;
    return fread(dst, 1, bytes, (FILE*)_fh);
}

unsigned int VFSFileReal::write(const void *src, unsigned int bytes)
{
    VFS_GUARD_OPT(this);
    if(!_fh)
        return npos;
    return fwrite(src, 1, bytes, (FILE*)_fh);
}

vfspos VFSFileReal::size(void)
{
    VFS_GUARD_OPT(this);
    if(_size != npos)
        return _size;
    open();
    close();
    // now size is known.
    return _size;
}

const void *VFSFileReal::getBuf(void)
{
    VFS_GUARD_OPT(this);
    if(_buf)
        return _buf;

    bool op = isopen();

    if(!op && !open()) // open with default params if not open
        return NULL;

    unsigned int s = (unsigned int)size();
    _buf = malloc(s + 4); // a bit extra padding
    if(!_buf)
        return NULL;

    if(op)
    {
        vfspos oldpos = getpos();
        seek(0);
        unsigned int offs = read(_buf, s);
        memset((char*)_buf + offs, 0, 4);
        seek(oldpos);
    }
    else
    {
        unsigned int offs = read(_buf, s);
        memset((char*)_buf + offs, 0, 4);
        close();
    }
    return _buf;
}

void VFSFileReal::dropBuf(bool del)
{
    VFS_GUARD_OPT(this);
    if(del && _buf)
        free(_buf);
    _buf = NULL;
}

// ------------- VFSFileMem -----------------------

VFSFileMem::VFSFileMem(const char *name, void *buf, unsigned int size, Mode mode /* = COPY */, delete_func delfunc /* = NULL */)
: VFSFile(), _pos(0), _size(size), _buf(buf), _delfunc(delfunc), _mybuf(mode == TAKE_OVER || mode == COPY)
{
    _setName(name);
    if(mode == COPY)
    {
        _buf = malloc(size+1);
        _delfunc = free;
        memcpy(_buf, buf, size);
        ((char*)_buf)[size] = 0;
    }
}

VFSFileMem::~VFSFileMem()
{
    if(_mybuf)
    {
        if(_delfunc)
            _delfunc(_buf);
        else
            delete [] (char*)_buf;
    }
}


void VFSFileMem::_setName(const char *n)
{
    if(n && *n)
    {
        _fullname = FixPath(n);
        _name = PathToFileName(_fullname.c_str());
    }
}

unsigned int VFSFileMem::read(void *dst, unsigned int bytes)
{
    VFS_GUARD_OPT(this);
    if(iseof())
        return 0;
    unsigned int rem = std::min<unsigned int>((unsigned int)(_size - _pos), bytes);

    memcpy(dst, (char*)_buf + _pos, rem);
    return rem;
}

unsigned int VFSFileMem::write(const void *src, unsigned int bytes)
{
    VFS_GUARD_OPT(this);
    if(iseof())
        return 0;
    unsigned int rem = std::min<unsigned int>((unsigned int)(_size - _pos), bytes);

    memcpy((char*)_buf + _pos, src, rem);
    return rem;
}

VFS_NAMESPACE_END
