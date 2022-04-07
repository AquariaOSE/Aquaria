# For building freetype2 in Aquaria
# Using a separate cmakefile for this since FT2 comes with its own CMakeLists.txt
# that we'd rather leave alone

OPTION(AQUARIA_INTERNAL_FREETYPE "Always use included FreeType library" TRUE)
if(AQUARIA_INTERNAL_FREETYPE)
    message(STATUS "Using internal copy of freetype2")
    SET(FREETYPE2DIR ${CMAKE_CURRENT_SOURCE_DIR}/freetype2)
    SET(FREETYPE2SRCDIR ${FREETYPE2DIR}/src)
    set(FREETYPE_INCLUDE_DIRS "${FREETYPE2DIR}/include" CACHE INTERNAL "")
    SET(FREETYPE2_SRCS
        ${FREETYPE2SRCDIR}/base/ftsystem.c
        ${FREETYPE2SRCDIR}/base/ftdebug.c
        ${FREETYPE2SRCDIR}/base/ftinit.c
        ${FREETYPE2SRCDIR}/base/ftbase.c
        ${FREETYPE2SRCDIR}/base/ftbbox.c
        ${FREETYPE2SRCDIR}/base/ftbdf.c
        ${FREETYPE2SRCDIR}/base/ftbitmap.c
        ${FREETYPE2SRCDIR}/base/ftcid.c
        ${FREETYPE2SRCDIR}/base/ftfstype.c
        ${FREETYPE2SRCDIR}/base/ftgasp.c
        ${FREETYPE2SRCDIR}/base/ftglyph.c
        ${FREETYPE2SRCDIR}/base/ftgxval.c
        ${FREETYPE2SRCDIR}/base/ftlcdfil.c
        ${FREETYPE2SRCDIR}/base/ftmm.c
        ${FREETYPE2SRCDIR}/base/ftotval.c
        ${FREETYPE2SRCDIR}/base/ftpatent.c
        ${FREETYPE2SRCDIR}/base/ftpfr.c
        ${FREETYPE2SRCDIR}/base/ftstroke.c
        ${FREETYPE2SRCDIR}/base/ftsynth.c
        ${FREETYPE2SRCDIR}/base/fttype1.c
        ${FREETYPE2SRCDIR}/base/ftwinfnt.c
        ${FREETYPE2SRCDIR}/truetype/truetype.c
        ${FREETYPE2SRCDIR}/type1/type1.c
        ${FREETYPE2SRCDIR}/cff/cff.c
        ${FREETYPE2SRCDIR}/cid/type1cid.c
        ${FREETYPE2SRCDIR}/pfr/pfr.c
        ${FREETYPE2SRCDIR}/type42/type42.c
        ${FREETYPE2SRCDIR}/winfonts/winfnt.c
        ${FREETYPE2SRCDIR}/pcf/pcf.c
        ${FREETYPE2SRCDIR}/bdf/bdf.c
        ${FREETYPE2SRCDIR}/sfnt/sfnt.c
        ${FREETYPE2SRCDIR}/autofit/autofit.c
        ${FREETYPE2SRCDIR}/pshinter/pshinter.c
        ${FREETYPE2SRCDIR}/raster/raster.c
        ${FREETYPE2SRCDIR}/smooth/smooth.c
        ${FREETYPE2SRCDIR}/cache/ftcache.c
        ${FREETYPE2SRCDIR}/gzip/ftgzip.c
        ${FREETYPE2SRCDIR}/lzw/ftlzw.c
        ${FREETYPE2SRCDIR}/psaux/psaux.c
        ${FREETYPE2SRCDIR}/psnames/psmodule.c
    )

    IF(MSVC)
        SET_SOURCE_FILES_PROPERTIES(
            ${FREETYPE2_SRCS}
            PROPERTIES COMPILE_FLAGS "-DFT_CONFIG_OPTION_SYSTEM_ZLIB -DFT2_BUILD_LIBRARY -I${FREETYPE2SRCDIR} -I${FREETYPE2DIR}/include/freetype/config -DHAVE_FCNTL_H"
        )
    ELSE(MSVC)
        # FT2 seems to not be strict-aliasing safe, so disable that in GCC.
        CHECK_C_COMPILER_FLAG("-fno-strict-aliasing" COMPILER_HAS_NOSTRICTALIAS)
        IF(COMPILER_HAS_NOSTRICTALIAS)
            SET(NOSTRICTALIAS "-fno-strict-aliasing")
        ELSE(COMPILER_HAS_NOSTRICTALIAS)
            SET(NOSTRICTALIAS "")
        ENDIF(COMPILER_HAS_NOSTRICTALIAS)
        SET_SOURCE_FILES_PROPERTIES(
            ${FREETYPE2_SRCS}
            PROPERTIES COMPILE_FLAGS "-Wno-extended-offsetof -DFT_CONFIG_OPTION_SYSTEM_ZLIB -DFT_CONFIG_CONFIG_H='\"${FREETYPE2DIR}/include/freetype/config/ftconfig.h\"' -DFT2_BUILD_LIBRARY -DFT_CONFIG_MODULES_H='\"${FREETYPE2DIR}/include/freetype/config/ftmodule.h\"' -I${FREETYPE2SRCDIR} -I${FREETYPE2DIR}/include/freetype/config -DHAVE_FCNTL_H ${NOSTRICTALIAS}"
        )
    ENDIF(MSVC)

    add_library(freetype ${FREETYPE2_SRCS})

else()

    find_package(Freetype REQUIRED)

endif()
