OPTION(AQUARIA_INTERNAL_OGGVORBIS "Always use included Ogg/Vorbis libraries" TRUE)
if(AQUARIA_INTERNAL_OGGVORBIS)
    message(STATUS "Using internal copy of ogg/vorbis")
    SET(LIBOGGDIR ${CMAKE_CURRENT_SOURCE_DIR}/libogg)
    SET(LIBVORBISDIR ${CMAKE_CURRENT_SOURCE_DIR}/libvorbis)
    set(OGGVORBIS_INCLUDE_DIRS "${LIBOGGDIR}/include;${LIBVORBISDIR}/include" CACHE INTERNAL "")
    include_directories(${OGGVORBIS_INCLUDE_DIRS})
    
    add_library(libogg
        ${LIBOGGDIR}/src/bitwise.c
        ${LIBOGGDIR}/src/framing.c
    )
    add_library(libvorbis
        ${LIBVORBISDIR}/lib/analysis.c
        ${LIBVORBISDIR}/lib/bitrate.c
        ${LIBVORBISDIR}/lib/block.c
        ${LIBVORBISDIR}/lib/codebook.c
        ${LIBVORBISDIR}/lib/envelope.c
        ${LIBVORBISDIR}/lib/floor0.c
        ${LIBVORBISDIR}/lib/floor1.c
        ${LIBVORBISDIR}/lib/info.c
        ${LIBVORBISDIR}/lib/lpc.c
        ${LIBVORBISDIR}/lib/lsp.c
        ${LIBVORBISDIR}/lib/mapping0.c
        ${LIBVORBISDIR}/lib/mdct.c
        ${LIBVORBISDIR}/lib/psy.c
        ${LIBVORBISDIR}/lib/registry.c
        ${LIBVORBISDIR}/lib/res0.c
        ${LIBVORBISDIR}/lib/sharedbook.c
        ${LIBVORBISDIR}/lib/smallft.c
        ${LIBVORBISDIR}/lib/synthesis.c
        ${LIBVORBISDIR}/lib/vorbisfile.c
        ${LIBVORBISDIR}/lib/window.c
    )
    target_link_libraries(libvorbis libogg)
else()
    # CMake doesn't seem to have a module for libogg or libvorbis yet, so
    # we roll our own based on existing find_package modules.

    find_path(OGG_INCLUDE_DIR ogg.h
        HINTS $ENV{OGG_DIR}
        PATH_SUFFIXES include/ogg include
        PATHS ~/Library/Frameworks /Library/Frameworks /usr/local /usr /sw /opt/local /opt/csw /opt
    )
    find_library(OGG_LIBRARY
        NAMES ogg
        HINTS $ENV{OGG_DIR}
        PATH_SUFFIXES lib64 lib
        PATHS ~/Library/Frameworks /Library/Frameworks /usr/local /usr /sw /opt/local /opt/csw /opt
    )

    if(OGG_LIBRARY)

        find_path(VORBIS_INCLUDE_DIR vorbisfile.h
            HINTS $ENV{VORBIS_DIR}
            PATH_SUFFIXES include/vorbis include
            PATHS ~/Library/Frameworks /Library/Frameworks /usr/local /usr /sw /opt/local /opt/csw /opt
        )
        find_library(VORBIS_LIBRARY
            NAMES vorbis
            HINTS $ENV{VORBIS_DIR}
            PATH_SUFFIXES lib64 lib
            PATHS ~/Library/Frameworks /Library/Frameworks /usr/local /usr /sw /opt/local /opt/csw /opt
        )
        find_library(VORBISFILE_LIBRARY
            NAMES vorbisfile
            HINTS $ENV{VORBIS_DIR}
            PATH_SUFFIXES lib64 lib
            PATHS ~/Library/Frameworks /Library/Frameworks /usr/local /usr /sw /opt/local /opt/csw /opt
        )

        if(VORBIS_LIBRARY AND VORBISFILE_LIBRARY)
            set(OGGVORBIS_INCLUDE_DIRS "${OGG_INCLUDE_DIR};${VORBIS_INCLUDE_DIR}" CACHE STRING "Ogg/Vorbis include directories")
            if(UNIX AND NOT APPLE)
                find_library(VORBIS_MATH_LIBRARY m)
                set(OGGVORBIS_LIBRARIES "${VORBISFILE_LIBRARY};${VORBIS_LIBRARY};${VORBIS_MATH_LIBRARY};${OGG_LIBRARY}" CACHE STRING "Ogg/Vorbis libraries")
            else(UNIX AND NOT APPLE)
                set(OGGVORBIS_LIBRARIES "${VORBISFILE_LIBRARY};${VORBIS_LIBRARY};${OGG_LIBRARY}" CACHE STRING "Ogg/Vorbis libraries")
            endif(UNIX AND NOT APPLE)
        endif(VORBIS_LIBRARY AND VORBISFILE_LIBRARY)

    endif(OGG_LIBRARY)

    find_package_handle_standard_args(OggVorbis  DEFAULT_MSG  OGGVORBIS_LIBRARIES OGGVORBIS_INCLUDE_DIRS)

    mark_as_advanced(OGG_INCLUDE_DIR VORBIS_INCLUDE_DIR OGGVORBIS_INCLUDE_DIRS)
    mark_as_advanced(OGG_LIBRARY VORBIS_LIBRARY VORBISFILE_LIBRARY VORBIS_MATH_LIBRARY OGGVORBIS_LIBRARIES)

endif()
