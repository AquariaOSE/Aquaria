
set(ttvfs_SRC
    ttvfs.h
    VFSArchiveLoader.h
    VFSBase.cpp
    VFSBase.h
    VFSDebug.cpp
    VFSDebug.h
    VFSDefines.h
    VFSDir.cpp
    VFSDir.h
    VFSDirInternal.cpp
    VFSDirInternal.h
    VFSDirView.cpp
    VFSDirView.h
    VFSFile.cpp
    VFSFile.h
    VFSFileFuncs.cpp
    VFSFileFuncs.h
    VFSInternal.h
    VFSLoader.cpp
    VFSLoader.h
    VFSRefcounted.h
    VFSRoot.h
    VFSRoot.cpp
    VFSSystemPaths.cpp
    VFSSystemPaths.h
    VFSTools.cpp
    VFSTools.h
)

add_library(ttvfs ${ttvfs_SRC})
