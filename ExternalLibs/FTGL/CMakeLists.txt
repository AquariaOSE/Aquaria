# For building Aquaria

include_directories(include)

set(FTGL_SRCS
    include/FTBBox.h
    include/FTCharmap.h
    include/FTCharToGlyphIndexMap.h
    include/FTFace.h
    include/FTFont.h
    include/FTGL.h
    include/FTGLTextureFont.h
    include/FTGlyph.h
    include/FTGlyphContainer.h
    include/FTLibrary.h
    include/FTPoint.h
    include/FTSize.h
    include/FTTextureGlyph.h
    include/FTVector.h
    src/FTCharmap.cpp
    src/FTFace.cpp
    src/FTFont.cpp
    src/FTGLTextureFont.cpp
    src/FTGlyph.cpp
    src/FTGlyphContainer.cpp
    src/FTLibrary.cpp
    src/FTPoint.cpp
    src/FTSize.cpp
    src/FTTextureGlyph.cpp
)

SET_SOURCE_FILES_PROPERTIES(
    ${FTGL_SRCS}
    PROPERTIES COMPILE_FLAGS "-DFTGL_LIBRARY_STATIC"
)

add_library(FTGL ${FTGL_SRCS})
target_link_libraries(FTGL freetype)
