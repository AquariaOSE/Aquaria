
set(ttvfs_zip_SRC
    VFSDirZip.cpp
    VFSDirZip.h
    VFSFileZip.cpp
    VFSFileZip.h
    VFSZipArchiveLoader.cpp
    VFSZipArchiveLoader.h
    VFSZipArchiveRef.cpp
    VFSZipArchiveRef.h
    ttvfs_zip.h
)

include_directories(${TTVFS_INCLUDE_DIRS})

add_library(ttvfs_zip ${ttvfs_zip_SRC})
