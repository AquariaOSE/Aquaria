#include "VFSFileZip.h"
#include "VFSDirZip.h"
#include "VFSTools.h"

#include "VFSInternal.h"

#include "miniz.h"

VFS_NAMESPACE_START


static size_t zip_read_func(void *pOpaque, mz_uint64 file_ofs, void *pBuf, size_t n)
{
    VFSFile *vf = (VFSFile*)pOpaque;
    mz_int64 cur_ofs = vf->getpos();
    if((mz_int64)file_ofs < 0)
        return 0;
    if(cur_ofs != (mz_int64)file_ofs && !vf->seek((vfspos)file_ofs))
        return 0;
    return vf->read(pBuf, n);
}

static bool zip_reader_init_vfsfile(mz_zip_archive *pZip, VFSFile *vf, mz_uint32 flags)
{
    if(!(pZip || vf))
        return false;
    vf->open("rb");
    mz_uint64 file_size = vf->size();
    if(!file_size)
    {
        vf->close();
        return false;
    }
    pZip->m_pRead = zip_read_func;
    pZip->m_pIO_opaque = vf;
    if (!mz_zip_reader_init(pZip, file_size, flags))
    {
        vf->close();
        return false;
    }
    return true;
}


VFSDirZip::VFSDirZip(VFSFile *zf)
: VFSDir(zf->fullname()), _zf(zf)
{
    _zf->ref++;
    _setOrigin(this);
    memset(&_zip, 0, sizeof(_zip));
}

VFSDirZip::~VFSDirZip()
{
    close();
    _zf->ref--;
}

bool VFSDirZip::close(void)
{
    mz_zip_reader_end(&_zip);
    _zf->close();
    return true;
}

VFSDir *VFSDirZip::createNew(const char *dir) const
{
    return VFSDir::createNew(dir); // inside a Zip file; only the base dir can be a real VFSDirZip.
}

unsigned int VFSDirZip::load(bool /*ignored*/)
{
    close();

    if(!zip_reader_init_vfsfile(&_zip, _zf, 0))
        return 0;

    unsigned int files = mz_zip_reader_get_num_files(&_zip);

    mz_zip_archive_file_stat fs;
    for (unsigned int i = 0; i < files; ++i)
    {
        // FIXME: do we want empty dirs in the tree?
        if(mz_zip_reader_is_file_a_directory(&_zip, i))
            continue;
        if(mz_zip_reader_is_file_encrypted(&_zip, i))
            continue;
        if(!mz_zip_reader_file_stat(&_zip, i, &fs))
            continue;
        if(getFile(fs.m_filename))
            continue;

        VFSFileZip *vf = new VFSFileZip(&_zip);
        vf->_setOrigin(this);
        memcpy(vf->getZipFileStat(), &fs, sizeof(mz_zip_archive_file_stat));
        vf->_init();
        addRecursive(vf, true, VFSDir::NONE);
        vf->ref--;
    }

    // Not necessary to keep open all the time, VFSFileZip will re-open the archive if needed
    //close();

    return files;
}

VFS_NAMESPACE_END