#define VFS_ENABLE_C_API 1

#include "ttvfs.h"
#include "ttvfs_stdio.h"
#include <cassert>
#include <sstream>


static ttvfs::Root *vfs = NULL;

void ttvfs_setroot(ttvfs::Root *root)
{
    vfs = root;
}

VFILE *vfopen(const char *fn, const char *mode)
{
    if (strchr(mode, 'w'))
    {
        assert(0 && "ttvfs_stdio: File writing via VFS not yet supported!");
        return NULL;
    }

    VFILE *vf = vfs->GetFile(fn);
    if (!vf || !vf->open(mode))
        return NULL;
    vf->incref(); // keep the file alive until closed.
    return vf;
}

size_t vfread(void *ptr, size_t size, size_t count, VFILE *vf)
{
    return vf->read(ptr, size * count) / size;
}

int vfclose(VFILE *vf)
{
    vf->close();
    vf->decref();
    return 0;
}

size_t vfwrite(const void *ptr, size_t size, size_t count, VFILE *vf)
{
    return vf->write(ptr, size * count) / size;
}

// return 0 on success, -1 on error
int vfseek(VFILE *vf, long int offset, int origin)
{
    return vf->seek(offset, origin) ? 0 : -1;
}

// warning: slow
char *vfgets(char *str, int num, VFILE *vf)
{
    char *s = str;
    if (vf->iseof())
        return NULL;
    unsigned int remain = int(vf->size() - vf->getpos());
    if (remain < (unsigned int)num)
        num = remain;
    else
        --num; // be sure to keep space for the final null char

    for(int i = 0; i < num; ++i)
    {
        char c;
        if(!vf->read(&c, 1))
            break;
        *s++ = c;
        if(c == '\n' || c == '\r')
            break;
    }

    *s++ = 0;
    return str;
}

int vfsize(VFILE *f, size_t *sizep)
{
    ttvfs::vfspos sz = f->size();
    if(sz == ttvfs::npos)
        return -1;
    *sizep = (size_t)sz;
    return 0;
}

long int vftell(VFILE *vf)
{
    return (long int)vf->getpos();
}


InStream::InStream(const std::string& fn)
: std::istringstream()
{
    open(fn.c_str());
}

InStream::InStream(const char *fn)
: std::istringstream()
{
    open(fn);
}

bool InStream::open(const char *fn)
{
    ttvfs::File *vf = vfs->GetFile(fn);
    if(vf && vf->open("r"))
    {
        size_t sz = (size_t)vf->size();
        std::string s;
        s.resize(sz);
        vf->read(&s[0], sz);
        str(s);
        vf->close();
        return true;
    }
    setstate(std::ios::failbit);
    return false;
}

int ttvfs_stdio_fsize(VFILE *f, size_t *sizep)
{
    size_t sz = 0;
    if (  vfseek(f, 0, SEEK_END) != 0
       || (sz = vftell(f)) < 0
       || vfseek(f, 0, SEEK_SET) != 0)
    {
        return -1;
    }

    *sizep = sz;
    return 0;
}

