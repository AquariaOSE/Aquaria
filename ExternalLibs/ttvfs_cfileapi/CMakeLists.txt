
set(cfileapi_SRC
    ttvfs_stdio.cpp
    ttvfs_stdio.h
)

include_directories(${TTVFS_INCLUDE_DIRS})

add_library(ttvfs_cfileapi ${cfileapi_SRC})

target_link_libraries(ttvfs_cfileapi ttvfs)

install(TARGETS ttvfs_cfileapi DESTINATION lib)

install(DIRECTORY ./ DESTINATION include/ttvfs
    FILES_MATCHING PATTERN "*.h")
