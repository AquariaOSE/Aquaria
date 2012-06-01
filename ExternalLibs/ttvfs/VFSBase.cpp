// VFSBase.cpp - common code for VFSDir and VFSFile
// For conditions of distribution and use, see copyright notice in VFS.h

#include "VFSBase.h"
#include "VFSInternal.h"
#include "VFSTools.h"

VFS_NAMESPACE_START

VFSBase::VFSBase()
: ref(this)
#ifdef VFS_USE_HASHMAP
  , _hash(0)
#endif
  , _origin(NULL)
{
}

// call this only with a lock held!
void VFSBase::_setName(const char *n)
{
    if(!n)
        return;
    _fullname = FixPath(n);
    _name = PathToFileName(_fullname.c_str());
#ifdef VFS_USE_HASHMAP
    _hash = STRINGHASH(_name);
#endif
}


VFS_NAMESPACE_END
