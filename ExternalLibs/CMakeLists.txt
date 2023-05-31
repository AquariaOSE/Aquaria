include(tinyxml2.cmake)
include(FTGL.cmake)
include(oggvorbis.cmake)
include(lua51.cmake)

INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/AL)
set(xdeps)

if(AQUARIA_USE_VFS)
    add_subdirectory(ttvfs)
    add_subdirectory(ttvfs_zip)
    add_subdirectory(ttvfs_cfileapi)
    set(xdeps ${xdeps} ttvfs ttvfs_zip ttvfs_cfileapi)
endif()

set(xsrc
    algorithmx.h
    ByteBuffer.h
    DeflateCompressor.cpp
    DeflateCompressor.h
    jps.hh
    luaalloc.cpp
    luaalloc.h
    minihttp.cpp
    minihttp.h
    minipstdint.h
    miniz.cpp
    miniz.h
    stb_image.h
    stb_image_resize.h
    stb_image_write.h
    tbsp.hh
    qoi.h
    tinylibs.cpp
    # Modified version, can't use external lib
    glfont2/glfont2.cpp
    glfont2/glfont2.h
)

if(AQUARIA_USE_SDL2 AND SDL2_FOUND)
    OPTION(AQUARIA_USE_MOJOAL "Use mojoAL instead of OpenAL (requires SDL2)" TRUE)
endif()

if(AQUARIA_USE_MOJOAL)
    set(xsrc ${xsrc} mojoal.c)
    set(OPENAL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/AL" CACHE PATH "OpenAL include directory" FORCE)
else()
    find_package(OpenAL REQUIRED)
    SET(xdeps ${xdeps} ${OPENAL_LIBRARY})
endif()



IF(WIN32)
    SET(xdeps ${xdeps} "ws2_32")
ENDIF(WIN32)

IF(HAIKU)
    SET(xdeps ${xdeps} "network")
ENDIF()

IF(APPLE)
    SET(xdeps ${xdeps} "-framework Carbon")
    SET(xdeps ${xdeps} "-framework Cocoa")
    #SET(xdeps ${xdeps} "-framework OpenAL")
ENDIF()

add_library(ExternalLibs ${xsrc})
target_link_libraries(ExternalLibs FTGL libogg tinyxml2 libvorbis ${SDL_LIBRARY} ${xdeps})
