#include "Core.h"
#include "VFSFileStream.h"

VFSTextStreamIn::VFSTextStreamIn(const std::string& fn, SimpleIStringStream::Mode strmode /* = TAKE_OVER*/)
: SimpleIStringStream()
{
    _init(fn.c_str(), strmode);
}

VFSTextStreamIn::VFSTextStreamIn(const char *fn, SimpleIStringStream::Mode strmode /* = TAKE_OVER*/)
: SimpleIStringStream()
{
    _init(fn, strmode);
}

void VFSTextStreamIn::_init(const char *fn, SimpleIStringStream::Mode strmode)
{
    ttvfs::VFSFile *vf = core->vfs.GetFile(fn);
    if(vf)
    {
        vf->open(NULL, "r");
        setString((char*)vf->getBuf(), strmode);
        vf->close();
        if(strmode == TAKE_OVER)
            vf->dropBuf(false);
    }
    else
        error = true;
}


VFSTextStdStreamIn::VFSTextStdStreamIn(const std::string& fn, SimpleIStringStream::Mode strmode /* = TAKE_OVER*/)
: std::istringstream()
{
    _init(fn.c_str(), strmode);
}

VFSTextStdStreamIn::VFSTextStdStreamIn(const char *fn, SimpleIStringStream::Mode strmode /* = TAKE_OVER*/)
: std::istringstream()
{
    _init(fn, strmode);
}

void VFSTextStdStreamIn::_init(const char *fn, SimpleIStringStream::Mode strmode)
{
    ttvfs::VFSFile *vf = core->vfs.GetFile(fn);
    if(vf)
    {
        vf->open(NULL, "r");
        str((char*)vf->getBuf()); // stringstream will always make a copy
        vf->close();
        if(strmode == SimpleIStringStream::TAKE_OVER)
            core->addVFSFileForDrop(vf);
    }
    else
        this->setstate(std::ios_base::failbit);
}
