#include "VFSFileZip.h"
#include "VFSDirZip.h"
#include "VFSTools.h"

#include "VFSInternal.h"

#include "miniz.h"

VFS_NAMESPACE_START



ZipDir::ZipDir(ZipArchiveRef *handle, const char *fullpath, bool canLoad)
: Dir(fullpath, NULL)
, _archiveHandle(handle)
, _canLoad(canLoad)
, _couldLoad(canLoad)
{
}

ZipDir::~ZipDir()
{
    close();
}


void ZipDir::close()
{
    _archiveHandle->close();
    _canLoad = _couldLoad; // allow loading again after re-opening (in case archive was replaced)
}

DirBase *ZipDir::createNew(const char *fullpath) const
{
    const ZipArchiveRef *czref = _archiveHandle;
    ZipArchiveRef *zref = const_cast<ZipArchiveRef*>(czref);
    return new ZipDir(zref, fullpath, false);
}

#define MZ ((mz_zip_archive*)_archiveHandle->mz)

void ZipDir::load()
{
    if(!_canLoad)
        return;

    _archiveHandle->openRead();

    const unsigned int files = mz_zip_reader_get_num_files(MZ);
    const size_t len = fullnameLen() + 1; // +1 for trailing '/' when used as path name in addRecursive()

    mz_zip_archive_file_stat fs;
    for (unsigned int i = 0; i < files; ++i)
    {
        if(mz_zip_reader_is_file_encrypted(MZ, i))
            continue;
        if(!mz_zip_reader_file_stat(MZ, i, &fs))
            continue;
        if(mz_zip_reader_is_file_a_directory(MZ, i))
        {
            _createAndInsertSubtree(fs.m_filename);
            continue;
        }
        if(getFile(fs.m_filename))
            continue;

        ZipFile *vf = new ZipFile(fs.m_filename, _archiveHandle, fs.m_file_index);
        _addRecursiveSkip(vf, len);
    }

    _canLoad = false;
}



VFS_NAMESPACE_END
