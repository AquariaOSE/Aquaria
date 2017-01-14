
set(ttvfs_zip_SRC
    VFSDirZip.cpp
    VFSDirZip.h
    VFSFileZip.cpp
    VFSFileZip.h
    VFSZipArchiveLoader.cpp
    VFSZipArchiveLoader.h
    VFSZipArchiveRef.cpp
    VFSZipArchiveRef.h
    miniz.c
    miniz.h
    ttvfs_zip.h
)

if(NOT MSVC)
    set_source_files_properties(
        miniz.c
        PROPERTIES COMPILE_FLAGS "-Wno-attributes -fno-strict-aliasing -Wno-strict-aliasing"
    )
endif(NOT MSVC)

include_directories(${TTVFS_INCLUDE_DIRS})

add_library(ttvfs_zip ${ttvfs_zip_SRC})

install(TARGETS ttvfs_zip DESTINATION lib)

install(FILES miniz.c DESTINATION include/ttvfs)

install(DIRECTORY ./ DESTINATION include/ttvfs
    FILES_MATCHING PATTERN "*.h")
