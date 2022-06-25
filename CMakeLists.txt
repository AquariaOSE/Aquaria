CMAKE_MINIMUM_REQUIRED(VERSION 2.6...3.20)
PROJECT(Aquaria)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

INCLUDE(CheckCCompilerFlag)
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckFunctionExists)

# if no build type was provided, set a default one
IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug, RelWithDebInfo, Release)" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# System fixups
IF(CMAKE_SYSTEM_NAME STREQUAL "Haiku")
    SET(HAIKU TRUE)
ENDIF()

IF(APPLE)
    SET(MACOSX TRUE)
ENDIF(APPLE)

# Recommended compiler flags that cmake doesn't set automatically
if(MSVC)
    # /MP: parallel builds
    # /GS-: disable security cookie (emits calls into vcrt)
    # /Oi: enable intrinsic functions
    # /fp:fast: -ffast-math
    set(AQUARIA_EXTRA_COMPILE_FLAGS "/MP /GS- /Oi /fp:fast" CACHE STRING "Extra compiler flags for MSVC")
    
    option(AQUARIA_MSVC_DEBUG_EDIT_AND_CONTINUE "MSVC: Enable edit+continue for debug builds?" TRUE)
    if(AQUARIA_MSVC_DEBUG_EDIT_AND_CONTINUE)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
    endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
    set(AQUARIA_EXTRA_COMPILE_FLAGS "-ffast-math" CACHE STRING "Extra compiler flags for GCC/Clang")
else()
    set(AQUARIA_EXTRA_COMPILE_FLAGS "" CACHE STRING "Extra compiler flags")
endif()

if(AQUARIA_EXTRA_COMPILE_FLAGS)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${AQUARIA_EXTRA_COMPILE_FLAGS}")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${AQUARIA_EXTRA_COMPILE_FLAGS}")
endif()


OPTION(AQUARIA_DEMO_BUILD "Demo Build?" FALSE)
OPTION(AQUARIA_USE_VFS "Use Virtual File System? Required for some additional features." TRUE)

OPTION(AQUARIA_USE_SDL2 "Use SDL2" TRUE)
OPTION(AQUARIA_USE_GLM "Use GLM for matrix math" TRUE)

OPTION(AQUARIA_DEBUG_SHOW_PATHS "Show important paths upon game start to aid in finding path problems" FALSE)
mark_as_advanced(AQUARIA_DEBUG_SHOW_PATHS)

################ Look for external libraries

### Pick one: SDL 1.2 or SDL2

if(AQUARIA_USE_SDL2)
    find_package(SDL2 REQUIRED)
    if(SDL2_FOUND)
        set(SDL_FOUND TRUE)
        set(SDL_INCLUDE_DIR ${SDL2_INCLUDE_DIR})
        set(SDL_LIBRARY ${SDL2_LIBRARY})
    endif(SDL2_FOUND)
else()
    find_package(SDL REQUIRED)
endif()

SET(BBGEDIR ${CMAKE_CURRENT_SOURCE_DIR}/BBGE)
SET(EXTLIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/ExternalLibs)

################ End of external libraries

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

if(AQUARIA_DEBUG_SHOW_PATHS)
    ADD_DEFINITIONS(-DAQUARIA_DEBUG_SHOW_PATHS)
endif(AQUARIA_DEBUG_SHOW_PATHS)

# Without #define VFS_ENABLE_C_API this is just stubbed out
include_directories(ttvfs_cfileapi)
if(AQUARIA_USE_VFS)
    ADD_DEFINITIONS(-DVFS_ENABLE_C_API=1)
    ADD_DEFINITIONS(-DBBGE_BUILD_VFS=1)
    INCLUDE_DIRECTORIES(${EXTLIBDIR}/ttvfs)
    INCLUDE_DIRECTORIES(${EXTLIBDIR}/ttvfs_zip)
    INCLUDE_DIRECTORIES(${EXTLIBDIR}/ttvfs_cfileapi)
ENDIF(AQUARIA_USE_VFS)

IF(AQUARIA_USE_GLM)
    ADD_DEFINITIONS(-DBBGE_USE_GLM=1)
ENDIF(AQUARIA_USE_GLM)

if(AQUARIA_INTERNAL_FTGL)
    ADD_DEFINITIONS(-DAQUARIA_INTERNAL_FTGL=1)
endif()

if(AQUARIA_INTERNAL_LUA)
    ADD_DEFINITIONS(-DAQUARIA_INTERNAL_LUA=1)
endif()

IF(AQUARIA_DEMO_BUILD)
    message(STATUS "Demo build.")
    ADD_DEFINITIONS(-DAQUARIA_DEMO=1)
ELSE(AQUARIA_DEMO_BUILD)
    ADD_DEFINITIONS(-DAQUARIA_BUILD_CONSOLE=1)
    ADD_DEFINITIONS(-DAQUARIA_BUILD_SCENEEDITOR=1)
ENDIF(AQUARIA_DEMO_BUILD)

IF(CMAKE_BUILD_TYPE STREQUAL "Release")
    ADD_DEFINITIONS(-DNDEBUG) # MSVC defines this in release mode by default, gcc/mingw do not
    message(STATUS "This is a release build.")
ENDIF(CMAKE_BUILD_TYPE STREQUAL "Release")
IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    ADD_DEFINITIONS(-D_DEBUG) # MSVC defines this in debug mode by default, gcc/mingw do not
    message(STATUS "This is a debug build.")
ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")

# FIXME: These should go
IF(UNIX)
    ADD_DEFINITIONS(-DBBGE_BUILD_UNIX=1)
ENDIF(UNIX)
IF(MACOSX)
    ADD_DEFINITIONS(-DBBGE_BUILD_MACOSX=1)
ENDIF(MACOSX)
IF(WIN32)
    ADD_DEFINITIONS(-DBBGE_BUILD_WINDOWS=1)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
ENDIF(WIN32)

IF(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -pipe -fsigned-char")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -pedantic -pipe -fsigned-char -std=gnu99")

    # See if -fno-stack-protector is available to us.
    # It doesn't seem to work well, and it adds bulk to the binary.
    CHECK_C_COMPILER_FLAG("-fno-stack-protector" AQUARIA_GCC_HAS_STACKPROT)
    IF(AQUARIA_GCC_HAS_STACKPROT)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-stack-protector")
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -fno-stack-protector")
    ENDIF(AQUARIA_GCC_HAS_STACKPROT)

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


add_subdirectory(ExternalLibs)

# Set external libs as deps for BBGE & Aquaria...
INCLUDE_DIRECTORIES(${BBGEDIR})
message(STATUS "FTGL_INCLUDE_DIRS: ${FTGL_INCLUDE_DIRS}")
INCLUDE_DIRECTORIES(${FTGL_INCLUDE_DIRS})
message(STATUS "LUA_INCLUDE_DIR: ${LUA_INCLUDE_DIR}")
INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR})
message(STATUS "OGGVORBIS_INCLUDE_DIRS: ${OGGVORBIS_INCLUDE_DIRS}")
INCLUDE_DIRECTORIES(${OGGVORBIS_INCLUDE_DIRS})
message(STATUS "SDL_INCLUDE_DIR: ${SDL_INCLUDE_DIR}")
INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
message(STATUS "OPENAL_INCLUDE_DIR: ${OPENAL_INCLUDE_DIR}")
INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR})
message(STATUS "TINYXML2_INCLUDE_DIRS: ${TINYXML2_INCLUDE_DIRS}")
INCLUDE_DIRECTORIES(${TINYXML2_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${EXTLIBDIR})


add_subdirectory(Aquaria)
add_subdirectory(BBGE)
