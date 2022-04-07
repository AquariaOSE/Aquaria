OPTION(AQUARIA_INTERNAL_FTGL "Always use included FTGL library" TRUE)
if(AQUARIA_INTERNAL_FTGL)
    message(STATUS "Using internal copy of FTGL")
    SET(FTGLDIR ${CMAKE_CURRENT_SOURCE_DIR}/FTGL)
    set(FTGL_INCLUDE_DIRS "${FTGLDIR}/include;${FREETYPE_INCLUDE_DIRS}" CACHE INTERNAL "")
    include(freetype2.cmake) # Nothing else uses freetype2 directly
    INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIRS})
    INCLUDE_DIRECTORIES(${EXTLIBDIR}) # For <GL/gl.h>
    add_subdirectory(FTGL)
else()
    find_package(FTGL REQUIRED)
endif()
