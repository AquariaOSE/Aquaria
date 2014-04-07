CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(Aquaria)

INCLUDE(CheckCCompilerFlag)
INCLUDE(CheckFunctionExists)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release CACHE STRING "None Debug Release RelWithDebInfo MinSizeRel" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

IF(APPLE)
    SET(MACOSX TRUE)
ENDIF(APPLE)

IF(CMAKE_SYSTEM_NAME STREQUAL "Haiku")
    SET(HAIKU TRUE)
ENDIF()

IF(WIN32)
    SET(WIN32_TRUE TRUE)
ELSE(WIN32)
    SET(WIN32_TRUE FALSE)
ENDIF(WIN32)

# if no build type was provided, set a default one
IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug, RelWithDebInfo, Release)" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

OPTION(AQUARIA_DEMO_BUILD "Demo Build?" FALSE)
OPTION(AQUARIA_USE_VFS "Use Virtual File System? Required for some additional features." TRUE)

OPTION(AQUARIA_USE_SDL2 "Use SDL2" FALSE)

# No Steamworks SDK for Linux at the moment. Roll our own achievements.
ADD_DEFINITIONS(-DBBGE_BUILD_ACHIEVEMENTS_INTERNAL=1)

SET(BBGEDIR ${CMAKE_CURRENT_SOURCE_DIR}/BBGE)
SET(SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/Aquaria)
SET(EXTLIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/ExternalLibs)

SET(FTGLDIR ${EXTLIBDIR}/FTGL)
SET(FTGLSRCDIR ${FTGLDIR}/src)
SET(FREETYPE2DIR ${EXTLIBDIR}/freetype2)
SET(FREETYPE2SRCDIR ${FREETYPE2DIR}/src)
SET(LUADIR ${EXTLIBDIR}/lua-5.1.4)
SET(LUASRCDIR ${LUADIR}/src)
SET(LIBOGGDIR ${EXTLIBDIR}/libogg-1.3.0)
SET(LIBVORBISDIR ${EXTLIBDIR}/libvorbis-1.3.3)
SET(ZLIBDIR ${EXTLIBDIR}/glpng/zlib)
SET(PNGDIR ${EXTLIBDIR}/glpng/png)

################ Look for external libraries

### zlib

OPTION(AQUARIA_INTERNAL_ZLIB "Always use included zlib library" TRUE)
if(NOT AQUARIA_INTERNAL_ZLIB)
    find_package(ZLIB)
endif(NOT AQUARIA_INTERNAL_ZLIB)
if(NOT ZLIB_FOUND)
    set(ZLIB_INCLUDE_DIRS ${ZLIBDIR})
endif(NOT ZLIB_FOUND)

### PNG

OPTION(AQUARIA_INTERNAL_PNG "Always use included PNG library" TRUE)
if(NOT AQUARIA_INTERNAL_PNG)
    find_package(PNG)
endif(NOT AQUARIA_INTERNAL_PNG)
if(NOT PNG_FOUND)
    set(PNG_PNG_INCLUDE_DIR ${PNGDIR})
    set(PNG_INCLUDE_DIR ${PNG_PNG_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS})
endif(NOT PNG_FOUND)

FILE(STRINGS "${PNG_PNG_INCLUDE_DIR}/png.h" PNG_H REGEX "^#define PNG_LIBPNG_VER[^0-9]*[0-9]+")
STRING(REGEX REPLACE "^.*PNG_LIBPNG_VER[^0-9]*([0-9]+).*$" "\\1" PNG_LIBPNG_VER "${PNG_H}")

### FreeType

OPTION(AQUARIA_INTERNAL_FREETYPE "Always use included FreeType library" TRUE)
if(NOT AQUARIA_INTERNAL_FREETYPE)
    find_package(Freetype)
endif(NOT AQUARIA_INTERNAL_FREETYPE)
if(NOT FREETYPE_FOUND)
    set(FREETYPE_INCLUDE_DIRS ${FREETYPE2DIR}/include)
endif(NOT FREETYPE_FOUND)

### Lua

OPTION(AQUARIA_INTERNAL_LUA "Always use included Lua library" TRUE)
if(NOT AQUARIA_INTERNAL_LUA)
    find_package(Lua51)
endif(NOT AQUARIA_INTERNAL_LUA)
if(NOT LUA51_FOUND)
    set(LUA_INCLUDE_DIR ${LUASRCDIR})
endif(NOT LUA51_FOUND)

### Ogg/Vorbis

OPTION(AQUARIA_INTERNAL_OGGVORBIS "Always use included Ogg/Vorbis libraries" TRUE)
if(NOT AQUARIA_INTERNAL_OGGVORBIS)
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

endif(NOT AQUARIA_INTERNAL_OGGVORBIS)

if(NOT OGGVORBIS_FOUND)
    set(OGGVORBIS_INCLUDE_DIRS ${LIBOGGDIR}/include ${LIBVORBISDIR}/include)
endif(NOT OGGVORBIS_FOUND)

### SDL

OPTION(AQUARIA_INTERNAL_SDL "Always use included SDL library" ${WIN32_TRUE})
if(NOT AQUARIA_INTERNAL_SDL)
    if(AQUARIA_USE_SDL2)
        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
        find_package(SDL2)
        if(SDL2_FOUND)
            set(SDL_FOUND TRUE)
            set(SDL_INCLUDE_DIR ${SDL2_INCLUDE_DIR})
            set(SDL_LIBRARY ${SDL2_LIBRARY})
        endif(SDL2_FOUND)
    else(AQUARIA_USE_SDL2)
        find_package(SDL)
    endif(AQUARIA_USE_SDL2)
endif(NOT AQUARIA_INTERNAL_SDL)
if(NOT SDL_FOUND)
    if(AQUARIA_USE_SDL2)
        set(SDLDIR "${EXTLIBDIR}/SDL2")
    else(AQUARIA_USE_SDL2)
        set(SDLDIR "${EXTLIBDIR}/SDL12")
    endif(AQUARIA_USE_SDL2)
    
    if(MACOSX)
        set(SDL_INCLUDE_DIR "${SDLDIR}/include")
        message(STATUS "Using internal copy of SDL")
        if(AQUARIA_USE_SDL2)
            set(SDL_LIBRARY "${SDLDIR}/lib/macosx/libSDL-2.0.0.dylib")
        else(AQUARIA_USE_SDL2)
            set(SDL_LIBRARY "${SDLDIR}/lib/macosx/libSDL-1.2.0.dylib;${SDLDIR}/lib/macosx/libSDLmain.a")
        endif(AQUARIA_USE_SDL2)
    elseif(WIN32)
        set(SDL_INCLUDE_DIR "${SDLDIR}/include" CACHE PATH "SDL include directory" FORCE)
        message(STATUS "Using internal copy of SDL")
        if(AQUARIA_USE_SDL2)
            set(SDLMAIN_LIBRARY "${SDLDIR}/lib/win32/SDL2main.lib" CACHE FILEPATH "Where the SDL2main library can be found" FORCE)
            set(SDL_LIBRARY "${SDLDIR}/lib/win32/SDL2.lib" CACHE FILEPATH "Where the SDL2 library can be found" FORCE)
        else(AQUARIA_USE_SDL2)
            set(SDLMAIN_LIBRARY "${SDLDIR}/lib/win32/SDLmain.lib" CACHE FILEPATH "Where the SDLmain library can be found" FORCE)
            set(SDL_LIBRARY "${SDLDIR}/lib/win32/SDL.lib" CACHE FILEPATH "Where the SDL library can be found" FORCE)
        endif(AQUARIA_USE_SDL2)
        set(SDL_LIBRARY ${SDLMAIN_LIBRARY} ${SDL_LIBRARY}) # not seen by user
    else(MACOSX)
        message(SEND_ERROR "We don't have a prebuilt SDL for this platform.")
    endif(MACOSX)
endif(NOT SDL_FOUND)

### OpenAL

OPTION(AQUARIA_INTERNAL_OPENAL "Always use included OpenAL library" ${WIN32_TRUE})
if(NOT AQUARIA_INTERNAL_OPENAL)
    find_package(OpenAL)
endif(NOT AQUARIA_INTERNAL_OPENAL)
if (NOT OPENAL_FOUND)
    if(WIN32)
        set(OPENALDIR "${EXTLIBDIR}/AL")
        set(OPENAL_INCLUDE_DIR "${OPENALDIR}/include" CACHE PATH "OpenAL include directory" FORCE)
        message(STATUS "Using internal copy of OpenAL")
        set(OPENAL_LIBRARY "${OPENALDIR}/lib/win32/OpenAL32.lib" CACHE FILEPATH "Where the OpenAL library can be found" FORCE)
    else(WIN32)
        message(SEND_ERROR "We don't have a prebuilt OpenAL for this platform.")
    endif(WIN32)
endif (NOT OPENAL_FOUND)

################ End of external libraries

INCLUDE_DIRECTORIES(${BBGEDIR})
INCLUDE_DIRECTORIES(${BBGEDIR}/GL)
INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${SRCDIR})
INCLUDE_DIRECTORIES(${FTGLDIR}/include)
INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${OGGVORBIS_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${EXTLIBDIR})



# Custom build ID: e.g. "-custom", " (my very own build)"
SET(AQUARIA_CUSTOM_BUILD_ID "" CACHE STRING
    "Text to append to the Aquaria version ID on the title screen.")
if (NOT(AQUARIA_CUSTOM_BUILD_ID STREQUAL ""))
  ADD_DEFINITIONS("-DAQUARIA_CUSTOM_BUILD_ID=\"${AQUARIA_CUSTOM_BUILD_ID}\"")
endif (NOT(AQUARIA_CUSTOM_BUILD_ID STREQUAL ""))

# Custom version string override (displayed as-is instead of "Aquaria vx.x.x ..." on the title screen
SET(AQUARIA_OVERRIDE_VERSION_STRING "" CACHE STRING
    "Text to display instead of the Aquaria version ID on the title screen. (Overrides AQUARIA_CUSTOM_BUILD_ID as well)")
if (NOT(AQUARIA_OVERRIDE_VERSION_STRING STREQUAL ""))
  ADD_DEFINITIONS("-DAQUARIA_OVERRIDE_VERSION_STRING=\"${AQUARIA_OVERRIDE_VERSION_STRING}\"")
endif (NOT(AQUARIA_OVERRIDE_VERSION_STRING STREQUAL ""))

# Custom data directories
SET(AQUARIA_DEFAULT_DATA_DIR "" CACHE STRING
    "Default data directory (for package maintainers only)")
if(NOT(AQUARIA_DEFAULT_DATA_DIR STREQUAL ""))
    ADD_DEFINITIONS("-DAQUARIA_DEFAULT_DATA_DIR=\"${AQUARIA_DEFAULT_DATA_DIR}\"")
endif(NOT(AQUARIA_DEFAULT_DATA_DIR STREQUAL ""))

SET(AQUARIA_EXTRA_DATA_DIR "" CACHE STRING
    "Extra data directory, overrides files from default datadir (for package maintainers only)")
if(NOT(AQUARIA_EXTRA_DATA_DIR STREQUAL ""))
    ADD_DEFINITIONS("-DAQUARIA_EXTRA_DATA_DIR=\"${AQUARIA_EXTRA_DATA_DIR}\"")
endif(NOT(AQUARIA_EXTRA_DATA_DIR STREQUAL ""))

# Aquaria/BBGE specific defines...

ADD_DEFINITIONS(-DGL_GLEXT_LEGACY=1)
ADD_DEFINITIONS(-DHAVE_PUTENV=1)
ADD_DEFINITIONS(-DTIXML_USE_STL=1)
ADD_DEFINITIONS(-DBBGE_SKIP_CONFIG_HEADERS=1) # if this is not defined, it will use .h files to set the necessary defines
ADD_DEFINITIONS(-DBBGE_BUILD_SDL=1)
ADD_DEFINITIONS(-DBBGE_BUILD_FRAMEBUFFER=1)
ADD_DEFINITIONS(-DBBGE_BUILD_SHADERS=1)
ADD_DEFINITIONS(-DBBGE_BUILD_OPENGL=1)
ADD_DEFINITIONS(-DBBGE_BUILD_OPENGL_DYNAMIC=1)
ADD_DEFINITIONS(-DBBGE_BUILD_FMOD_OPENAL_BRIDGE=1)
IF(AQUARIA_USE_VFS)
    ADD_DEFINITIONS(-DBBGE_BUILD_VFS=1)
    ADD_DEFINITIONS(-VFS_ENABLE_C_API=1)
    INCLUDE_DIRECTORIES(${EXTLIBDIR}/ttvfs)
    INCLUDE_DIRECTORIES(${EXTLIBDIR}/ttvfs_zip)
    INCLUDE_DIRECTORIES(${EXTLIBDIR}/ttvfs_cfileapi)
ENDIF(AQUARIA_USE_VFS)

IF(AQUARIA_USE_SDL2)
    ADD_DEFINITIONS(-DBBGE_BUILD_SDL2=1)
ENDIF(AQUARIA_USE_SDL2)

IF(AQUARIA_DEMO_BUILD)
    message(STATUS "Demo build.")
    ADD_DEFINITIONS(-DAQUARIA_DEMO=1)
ELSE(AQUARIA_DEMO_BUILD)
    ADD_DEFINITIONS(-DAQUARIA_BUILD_CONSOLE=1)
    ADD_DEFINITIONS(-DAQUARIA_BUILD_SCENEEDITOR=1)
ENDIF(AQUARIA_DEMO_BUILD)

IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    ADD_DEFINITIONS(-D_DEBUG) # MSVC defines this in debug mode by default, gcc/mingw do not
    message(STATUS "This is a debug build.")
ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")

IF(UNIX)
    ADD_DEFINITIONS(-DBBGE_BUILD_UNIX=1)
ENDIF(UNIX)

IF(MACOSX)
    ADD_DEFINITIONS(-DBBGE_BUILD_MACOSX=1)
ENDIF(MACOSX)

IF(WIN32)
    ADD_DEFINITIONS(-DBBGE_BUILD_WINDOWS=1)
    SET(EXETYPE WIN32)
    SET(OPTIONAL_SRCS ${OPTIONAL_SRCS} aquaria.rc)
ENDIF(WIN32)


# Build Lua with Unix _setjmp/_longjmp support.
IF(UNIX AND NOT HAIKU)
    ADD_DEFINITIONS(-DLUA_USE_ULONGJMP=1)
ENDIF()

IF(CMAKE_COMPILER_IS_GNUCC)
    ADD_DEFINITIONS(-pipe -fsigned-char)
    #ADD_DEFINITIONS(-Wall)

    # See if -fno-stack-protector is available to us.
    # It doesn't seem to work well, and it adds bulk to the binary.
    CHECK_C_COMPILER_FLAG("-fno-stack-protector" AQUARIA_GCC_HAS_STACKPROT)
    IF(AQUARIA_GCC_HAS_STACKPROT)
        ADD_DEFINITIONS(-fno-stack-protector)
    ENDIF(AQUARIA_GCC_HAS_STACKPROT)
    
    # -O3 breaks on some GCC/MinGW versions, make sure CMake does not set this as default.
    # Exceptions are not used, excluding support for release builds adds less bulk as well.
    set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Flags used for release builds" FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -fno-exceptions" CACHE STRING "Flags used for release builds" FORCE)

    # !!! FIXME: probably not safe long-term.
    # CMake mailing list had this hack for getting rid of -rdynamic:
    #   http://public.kitware.com/pipermail/cmake/2006-July/010404.html
    IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
        SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
    ENDIF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

CHECK_FUNCTION_EXISTS(strcasecmp HAVE_STRCASECMP)
IF(HAVE_STRCASECMP)
    ADD_DEFINITIONS(-DHAVE_STRCASECMP)
ENDIF(HAVE_STRCASECMP)

# Main game source code for Aquaria, minus engine and other middleware...
SET(AQUARIA_SRCS
    ${SRCDIR}/AnimationEditor.cpp
    ${SRCDIR}/AquariaComboBox.cpp
    ${SRCDIR}/AquariaMenuItem.cpp
    ${SRCDIR}/AquariaProgressBar.cpp
    ${SRCDIR}/AquariaSaveSlot.cpp
    ${SRCDIR}/AutoMap.cpp
    ${SRCDIR}/Avatar.cpp
    ${SRCDIR}/Beam.cpp
    ${SRCDIR}/BitBlotLogo.cpp
    ${SRCDIR}/CollideEntity.cpp
    ${SRCDIR}/Continuity.cpp
    ${SRCDIR}/Credits.cpp
    ${SRCDIR}/CurrentRender.cpp
    ${SRCDIR}/Demo.cpp
    ${SRCDIR}/DSQ.cpp
    ${SRCDIR}/Element.cpp
    ${SRCDIR}/Emote.cpp
    ${SRCDIR}/Entity.cpp
    ${SRCDIR}/FlockEntity.cpp
    ${SRCDIR}/Game.cpp
    ${SRCDIR}/GameplayVariables.cpp
    ${SRCDIR}/GasCloud.cpp
    ${SRCDIR}/GridRender.cpp
    ${SRCDIR}/Hair.cpp
    ${SRCDIR}/Ingredient.cpp
    ${SRCDIR}/Intro.cpp
    ${SRCDIR}/Logo.cpp
    ${SRCDIR}/Main.cpp
    ${SRCDIR}/ManaBall.cpp
    ${SRCDIR}/MiniMapRender.cpp
    ${SRCDIR}/Mod.cpp
    ${SRCDIR}/ModSelector.cpp
    ${SRCDIR}/ModDownloader.cpp
    ${SRCDIR}/Network.cpp
    ${SRCDIR}/ParticleEditor.cpp
    ${SRCDIR}/Path.cpp
    ${SRCDIR}/PathFinding.cpp
    ${SRCDIR}/PathRender.cpp
    ${SRCDIR}/RecipeMenuEntry.cpp
    ${SRCDIR}/SceneEditor.cpp
    ${SRCDIR}/SchoolFish.cpp
    ${SRCDIR}/ScriptedEntity.cpp
    ${SRCDIR}/ScriptInterface.cpp
    ${SRCDIR}/Segmented.cpp
    ${SRCDIR}/SFXLoops.cpp
    ${SRCDIR}/Shot.cpp
    ${SRCDIR}/Spore.cpp
    ${SRCDIR}/States.cpp
    ${SRCDIR}/StatsAndAchievements.cpp
    ${SRCDIR}/SteamRender.cpp
    ${SRCDIR}/Strand.cpp
    ${SRCDIR}/StringBank.cpp
    ${SRCDIR}/SubtitlePlayer.cpp
    ${SRCDIR}/ToolTip.cpp
    ${SRCDIR}/UserSettings.cpp
    ${SRCDIR}/WaterSurfaceRender.cpp
    ${SRCDIR}/Web.cpp
    ${SRCDIR}/WorldMapRender.cpp
    ${SRCDIR}/WorldMapTiles.cpp
)

# Apparently not used at the moment. Listed here just for completeness.
SET(AQUARIA_SRCS_UNUSED
    ${SRCDIR}/BoxElement.cpp
    ${SRCDIR}/BubbleRender.cpp
    ${SRCDIR}/FFTNotes.cpp
    ${SRCDIR}/StarRenderer.cpp
    ${SRCDIR}/WaterFont.cpp
)

IF(MACOSX)
    IF(AQUARIA_USE_SDL)
        SET(COCOA_SRCS "${BBGEDIR}/Cocoa.mm")
    ENDIF(AQUARIA_USE_SDL)
ENDIF(MACOSX)

# Bit Blot Game Engine sources...
SET(BBGE_SRCS
    ${BBGEDIR}/ActionInput.cpp
    ${BBGEDIR}/ActionMapper.cpp
    ${BBGEDIR}/ActionSet.cpp
    ${BBGEDIR}/AfterEffect.cpp
    ${BBGEDIR}/Base.cpp
    ${BBGEDIR}/BitmapFont.cpp
    ${BBGEDIR}/Collision.cpp
    ${BBGEDIR}/Core.cpp
    ${BBGEDIR}/Cube.cpp
    ${BBGEDIR}/DarkLayer.cpp
    ${BBGEDIR}/DebugFont.cpp
    ${BBGEDIR}/Effects.cpp
    ${BBGEDIR}/Emitter.cpp
    ${BBGEDIR}/Event.cpp
    ${BBGEDIR}/Flags.cpp
    ${BBGEDIR}/FrameBuffer.cpp
    ${BBGEDIR}/Gradient.cpp
    ${BBGEDIR}/Joystick.cpp
    ${BBGEDIR}/LensFlare.cpp
    ${BBGEDIR}/Localization.cpp
    ${BBGEDIR}/Math.cpp
    ${BBGEDIR}/MemoryAllocatorSmallBlock.cpp
    ${BBGEDIR}/MT.cpp
    ${BBGEDIR}/ParticleEffect.cpp
    ${BBGEDIR}/ParticleManager.cpp
    ${BBGEDIR}/Particles.cpp
    ${BBGEDIR}/Precacher.cpp
    ${BBGEDIR}/ProfRender.cpp
    ${BBGEDIR}/Quad.cpp
    ${BBGEDIR}/QuadTrail.cpp
    ${BBGEDIR}/RenderObject.cpp
    ${BBGEDIR}/RenderObjectLayer.cpp
    ${BBGEDIR}/RenderRect.cpp
    ${BBGEDIR}/Resource.cpp
    ${BBGEDIR}/RoundedRect.cpp
    ${BBGEDIR}/ScreenTransition.cpp
    ${BBGEDIR}/ScriptObject.cpp
    ${BBGEDIR}/Shader.cpp
    ${BBGEDIR}/SkeletalSprite.cpp
    ${BBGEDIR}/Slider.cpp
    ${BBGEDIR}/SoundManager.cpp
    ${BBGEDIR}/SpawnParticleData.cpp
    ${BBGEDIR}/StateMachine.cpp
    ${BBGEDIR}/StateManager.cpp
    ${BBGEDIR}/Strings.cpp
    ${BBGEDIR}/Texture.cpp
    ${BBGEDIR}/TTFFont.cpp
    ${BBGEDIR}/Vector.cpp
    ${BBGEDIR}/FmodOpenALBridge.cpp
    ${COCOA_SRCS}
    ${EXTLIBDIR}/DeflateCompressor.cpp
    ${EXTLIBDIR}/FileAPI.cpp
    ${EXTLIBDIR}/glfont2/glfont2.cpp
    ${EXTLIBDIR}/glpng/glpng.c
    ${EXTLIBDIR}/tinyxml.cpp
    ${EXTLIBDIR}/tinyxmlerror.cpp
    ${EXTLIBDIR}/tinyxmlparser.cpp
    ${EXTLIBDIR}/FileAPI.cpp
    ${EXTLIBDIR}/minihttp.cpp
    ${EXTLIBDIR}/JPS.h
)

SET(ZLIB_SRCS
    ${ZLIBDIR}/adler32.c
    ${ZLIBDIR}/crc32.c
    ${ZLIBDIR}/compress.c
    ${ZLIBDIR}/deflate.c
    ${ZLIBDIR}/inffast.c
    ${ZLIBDIR}/inflate.c
    ${ZLIBDIR}/inftrees.c
    ${ZLIBDIR}/trees.c
    ${ZLIBDIR}/uncompr.c
    ${ZLIBDIR}/zutil.c
)

SET(PNG_SRCS
    ${PNGDIR}/png.c
    ${PNGDIR}/pngerror.c
    ${PNGDIR}/pngget.c
    ${PNGDIR}/pngmem.c
    ${PNGDIR}/pngpread.c
    ${PNGDIR}/pngread.c
    ${PNGDIR}/pngrio.c
    ${PNGDIR}/pngrtran.c
    ${PNGDIR}/pngrutil.c
    ${PNGDIR}/pngset.c
    ${PNGDIR}/pngtrans.c
)

# Apparently not used at the moment. Listed here just for completeness.
SET(BBGE_SRCS_UNUSED
    ${BBGEDIR}/AnimatedSprite.cpp
    ${BBGEDIR}/BloomEffect.cpp
    ${BBGEDIR}/CShim.cpp
    ${BBGEDIR}/Cutscene.cpp
    ${BBGEDIR}/Datafile.cpp
    ${BBGEDIR}/DFSprite.cpp
    ${BBGEDIR}/FileVars.cpp
    ${BBGEDIR}/Interpolator.cpp
    ${BBGEDIR}/Light.cpp
    ${BBGEDIR}/LightCone.cpp
    ${BBGEDIR}/Model.cpp
    ${BBGEDIR}/OggStream.cpp
    ${BBGEDIR}/PackRead.cpp
    ${BBGEDIR}/PointSprites.cpp
)

IF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
SET_SOURCE_FILES_PROPERTIES(
    ${BBGEDIR}/MT.cpp
    PROPERTIES COMPILE_FLAGS "-fpermissive"
)
ENDIF()

SET(FTGL_SRCS
    ${FTGLSRCDIR}/FTCharmap.cpp
    ${FTGLSRCDIR}/FTContour.cpp
    ${FTGLSRCDIR}/FTFace.cpp
    ${FTGLSRCDIR}/FTFont.cpp
    ${FTGLSRCDIR}/FTGLTextureFont.cpp
    ${FTGLSRCDIR}/FTGlyph.cpp
    ${FTGLSRCDIR}/FTGlyphContainer.cpp
    ${FTGLSRCDIR}/FTLibrary.cpp
    ${FTGLSRCDIR}/FTPoint.cpp
    ${FTGLSRCDIR}/FTSize.cpp
    ${FTGLSRCDIR}/FTTextureGlyph.cpp
)

SET_SOURCE_FILES_PROPERTIES(
    ${FTGL_SRCS}
    PROPERTIES COMPILE_FLAGS "-DFTGL_LIBRARY_STATIC"
)

SET(OGGVORBIS_SRCS
    ${LIBOGGDIR}/src/bitwise.c
    ${LIBOGGDIR}/src/framing.c
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
    ${FREETYPE2SRCDIR}/base/ftxf86.c
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
        PROPERTIES COMPILE_FLAGS "-DFT_CONFIG_OPTION_SYSTEM_ZLIB -DFT_CONFIG_CONFIG_H='\"${FREETYPE2DIR}/include/freetype/config/ftconfig.h\"' -DFT2_BUILD_LIBRARY -DFT_CONFIG_MODULES_H='\"${FREETYPE2DIR}/include/freetype/config/ftmodule.h\"' -I${FREETYPE2SRCDIR} -I${FREETYPE2DIR}/include/freetype/config -DHAVE_FCNTL_H ${NOSTRICTALIAS}"
    )
ENDIF(MSVC)


SET(LUA_SRCS
    ${LUASRCDIR}/lapi.c
    ${LUASRCDIR}/ldblib.c
    ${LUASRCDIR}/ldebug.c
    ${LUASRCDIR}/ldo.c
    ${LUASRCDIR}/ldump.c
    ${LUASRCDIR}/lfunc.c
    ${LUASRCDIR}/lgc.c
    ${LUASRCDIR}/lmem.c
    ${LUASRCDIR}/lobject.c
    ${LUASRCDIR}/lopcodes.c
    ${LUASRCDIR}/lstate.c
    ${LUASRCDIR}/lstring.c
    ${LUASRCDIR}/ltable.c
    ${LUASRCDIR}/ltm.c
    ${LUASRCDIR}/lundump.c
    ${LUASRCDIR}/lvm.c
    ${LUASRCDIR}/lzio.c
    ${LUASRCDIR}/lauxlib.c
    ${LUASRCDIR}/lbaselib.c
    ${LUASRCDIR}/lstrlib.c
    ${LUASRCDIR}/ltablib.c
    ${LUASRCDIR}/lparser.c
    ${LUASRCDIR}/llex.c
    ${LUASRCDIR}/lcode.c
    ${LUASRCDIR}/lmathlib.c
)

IF(AQUARIA_USE_VFS)
    ADD_SUBDIRECTORY("${EXTLIBDIR}/ttvfs")
    ADD_SUBDIRECTORY("${EXTLIBDIR}/ttvfs_zip")
    ADD_SUBDIRECTORY("${EXTLIBDIR}/ttvfs_cfileapi")
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} "ttvfs")
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} "ttvfs_zip")
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} "ttvfs_cfileapi")
ENDIF(AQUARIA_USE_VFS)

IF(WIN32)
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} "ws2_32")
ENDIF(WIN32)

IF(MACOSX)
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} "-framework Carbon")
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} "-framework Cocoa")
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} "-framework OpenAL")
ENDIF(MACOSX)

IF(HAIKU)
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} "network")
ENDIF()

SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} ${SDL_LIBRARY})
SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} ${OPENAL_LIBRARY})

IF(ZLIB_FOUND)
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} ${ZLIB_LIBRARIES})
ELSE(ZLIB_FOUND)
    SET(OPTIONAL_SRCS ${OPTIONAL_SRCS} ${ZLIB_SRCS})
ENDIF(ZLIB_FOUND)

IF(PNG_FOUND)
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} ${PNG_LIBRARIES})
ELSE(PNG_FOUND)
    SET(OPTIONAL_SRCS ${OPTIONAL_SRCS} ${PNG_SRCS})
ENDIF(PNG_FOUND)

IF(FREETYPE_FOUND)
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} ${FREETYPE_LIBRARIES})
ELSE(FREETYPE_FOUND)
    SET(OPTIONAL_SRCS ${OPTIONAL_SRCS} ${FREETYPE2_SRCS})
ENDIF(FREETYPE_FOUND)

IF(LUA51_FOUND)
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} ${LUA_LIBRARIES})
ELSE(LUA51_FOUND)
    SET(OPTIONAL_SRCS ${OPTIONAL_SRCS} ${LUA_SRCS})
ENDIF(LUA51_FOUND)

IF(OGGVORBIS_FOUND)
    SET(OPTIONAL_LIBS ${OPTIONAL_LIBS} ${OGGVORBIS_LIBRARIES})
ELSE(OGGVORBIS_FOUND)
    SET(OPTIONAL_SRCS ${OPTIONAL_SRCS} ${OGGVORBIS_SRCS})
ENDIF(OGGVORBIS_FOUND)

ADD_EXECUTABLE(aquaria ${EXETYPE}
    ${AQUARIA_SRCS}
    ${BBGE_SRCS}
    ${FTGL_SRCS}
    ${OPTIONAL_SRCS}
)
TARGET_LINK_LIBRARIES(aquaria ${OPTIONAL_LIBS})

# end of CMakeLists.txt ...

