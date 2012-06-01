
set(ttvfs_SRC
    VFS.h
    VFSArchiveLoader.h
    VFSAtomic.cpp
    VFSAtomic.h
    VFSBase.cpp
    VFSBase.h
    VFSDefines.h
    VFSDir.cpp
    VFSDir.h
    VFSFile.cpp
    VFSFile.h
    VFSFileFuncs.cpp
    VFSFileFuncs.h
    VFSHashmap.h
    VFSHelper.cpp
    VFSHelper.h
    VFSInternal.h
    VFSLoader.cpp
    VFSLoader.h
    VFSSelfRefCounter.h
    VFSSystemPaths.cpp
    VFSSystemPaths.h
    VFSTools.cpp
    VFSTools.h
)

add_library(ttvfs ${ttvfs_SRC})
