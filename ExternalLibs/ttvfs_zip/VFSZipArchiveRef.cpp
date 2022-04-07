#include "VFSInternal.h"
#include "VFSZipArchiveRef.h"
#include <stdio.h>
#include <miniz.h>


VFS_NAMESPACE_START


static size_t zip_read_func(void *pOpaque, mz_uint64 file_ofs, void *pBuf, size_t n)
{
    File *vf = (File*)pOpaque;
    mz_int64 cur_ofs = vf->getpos();
    if((mz_int64)file_ofs < 0)
        return 0;
    if(cur_ofs != (mz_int64)file_ofs && !vf->seek((vfspos)file_ofs, SEEK_SET))
        return 0;
    return vf->read(pBuf, n);
}

static bool zip_reader_init_vfsfile(mz_zip_archive *pZip, File *vf, mz_uint32 flags)
{
    mz_uint64 file_size = vf->size();
    if(!file_size || !vf->open("rb"))
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

static bool zip_reader_reopen_vfsfile(mz_zip_archive *pZip, mz_uint32 flags)
{
    if(!(pZip && pZip->m_pIO_opaque && pZip->m_pRead))
        return false;
    File *vf = (File*)pZip->m_pIO_opaque;
    mz_uint64 file_size = vf->size();
    if(!file_size)
    {
        vf->close();
        return false;
    }
    if(!vf->isopen())
        if(!vf->open("rb"))
            return false;
    if(pZip->m_zip_mode == MZ_ZIP_MODE_READING)
        return true;
    if (!mz_zip_reader_init(pZip, file_size, flags))
    {
        vf->close();
        return false;
    }
    return true;
}




ZipArchiveRef::ZipArchiveRef(File *file)
: archiveFile(file)
{
    mz = new mz_zip_archive;
    memset(mz, 0, sizeof(mz_zip_archive));
}

#define MZ ((mz_zip_archive*)mz)

ZipArchiveRef::~ZipArchiveRef()
{
    close();
    delete MZ;
}

bool ZipArchiveRef::init()
{
    return zip_reader_init_vfsfile(MZ, archiveFile, 0);
}

bool ZipArchiveRef::openRead()
{
    return zip_reader_reopen_vfsfile(MZ, 0);
}

void ZipArchiveRef::close()
{
    switch(MZ->m_zip_mode)
    {
        case MZ_ZIP_MODE_READING:
            mz_zip_reader_end(MZ);
            break;

        case MZ_ZIP_MODE_WRITING:
            mz_zip_writer_finalize_archive(MZ);
            mz_zip_writer_end(MZ);
            break;

        case MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED:
            mz_zip_writer_end(MZ);
            break;

        case MZ_ZIP_MODE_INVALID:
            break; // nothing to do
    }

    archiveFile->close();
}

const char *ZipArchiveRef::fullname() const
{
    return archiveFile->fullname();
}



VFS_NAMESPACE_END

