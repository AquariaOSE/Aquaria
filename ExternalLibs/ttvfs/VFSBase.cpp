// VFSBase.cpp - common code for VFSDir and VFSFile
// For conditions of distribution and use, see copyright notice in VFS.h

#include "VFSBase.h"
#include "VFSInternal.h"
#include "VFSTools.h"

VFS_NAMESPACE_START

VFSBase::VFSBase()
{
}

void VFSBase::_setName(const char *n)
{
    if(!n)
        return;
    _fullname = n;
    FixPath(_fullname);
    _name = GetBaseNameFromPath(_fullname.c_str());
}


VFS_NAMESPACE_END
