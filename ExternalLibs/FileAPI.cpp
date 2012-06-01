#ifdef BBGE_BUILD_VFS

#include "FileAPI.h"
#include "ttvfs_zip/VFSZipArchiveLoader.h"
#include <stdio.h>


ttvfs::VFSHelper vfs;


VFILE *vfopen(const char *fn, const char *mode)
{
	if (strchr(mode, 'w'))
	{
		fprintf(stderr, "FileAPI.h: File writing via VFS not yet supported!");
		return NULL;
	}

	VFILE *vf = vfs.GetFile(fn);
	if (!vf || !vf->open(mode))
		return NULL;
	++(vf->ref); // keep the file alive until closed.
	return vf;
}

size_t vfread(void *ptr, size_t size, size_t count, VFILE *vf)
{
	return vf->read(ptr, size * count) / size;
}

int vfclose(VFILE *vf)
{
	bool closed = vf->close();
	vf->ref--;
	return closed ? 0 : EOF;
}

size_t vfwrite(const void *ptr, size_t size, size_t count, VFILE *vf)
{
	return vf->write(ptr, size * count) / size;
}

// return 0 on success, -1 on error
int vfseek(VFILE *vf, long int offset, int origin)
{
	bool ok = false;
	switch(origin)
	{
	case SEEK_SET: ok = vf->seek(offset); break;
	case SEEK_CUR: ok = vf->seekRel(offset); break;
	case SEEK_END: ok = vf->seek((long int)(vf->size() - offset)); break;
	}
	return ok ? 0 : -1;
}

char *vfgets(char *str, int num, VFILE *vf)
{
	char *s = str;
	if (vf->iseof())
		return NULL;
	char *ptr = (char*)vf->getBuf() + vf->getpos();
	unsigned int remain = int(vf->size() - vf->getpos());
	if (remain < (unsigned int)num)
		num = remain;
	else
		--num; // be sure to keep space for the final null char
	int i = 0;
	char c;
	for( ; i < num && *ptr; ++i)
	{
		c = (*s++ = *ptr++);
		if(c == '\n' || c == '\r')
		{
			++i;
			c = *ptr++; // because windows linebreaks suck.
			if(c == '\n' || c == '\r')
				++i;
			break;
		}
	}

	vf->seekRel(i);
	*s++ = 0;
	return str;
}

void vfclear(VFILE *vf)
{
	vf->dropBuf(true);
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
    ttvfs::VFSFile *vf = vfs.GetFile(fn);
    if(vf)
    {
        vf->open("r");
        str((char*)vf->getBuf()); // stringstream will always make a copy
        vf->close();
        vf->dropBuf(true);
        return true;
    }
    setstate(std::ios_base::failbit);
    return false;
}

#endif
