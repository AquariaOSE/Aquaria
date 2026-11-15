# For building in Aquaria

set(lua51_src
    src/lapi.c
    src/lapi.h
    src/lauxlib.c
    src/lauxlib.h
    src/lbaselib.c
    src/lcode.c
    src/lcode.h
    src/ldblib.c
    src/ldebug.c
    src/ldebug.h
    src/ldo.c
    src/ldo.h
    src/ldump.c
    src/lfunc.c
    src/lfunc.h
    src/lgc.c
    src/lgc.h
    src/linit.c
    src/liolib.c
    src/llex.c
    src/llex.h
    src/llimits.h
    src/lmathlib.c
    src/lmem.c
    src/lmem.h
    src/loadlib.c
    src/lobject.c
    src/lobject.h
    src/lopcodes.c
    src/lopcodes.h
    src/loslib.c
    src/lparser.c
    src/lparser.h
    src/lstate.c
    src/lstate.h
    src/lstring.c
    src/lstring.h
    src/lstrlib.c
    src/ltable.c
    src/ltable.h
    src/ltablib.c
    src/ltm.c
    src/ltm.h
#   src/lua.c
    src/lua.h
#   src/luac.c
    src/luaconf.h
    src/lualib.h
    src/lundump.c
    src/lundump.h
    src/lvm.c
    src/lvm.h
    src/lzio.c
    src/lzio.h
    src/print.c
)

# Use C++ exceptions instead of setjmp() & longjmp()
set_source_files_properties(${lua51_src} PROPERTIES LANGUAGE CXX)

# Windows is detected automatically
if(LINUX)
    ADD_DEFINITIONS(-DLUA_USE_LINUX)
elseif(APPLE)
     ADD_DEFINITIONS(-DLUA_USE_MACOSX)
elseif(POSIX)
    ADD_DEFINITIONS(-DLUA_USE_POSIX)
else()
    # gcc/ld warns about tmpnam() so we might as well try to get rid of it
    CHECK_FUNCTION_EXISTS(mkstemp HAVE_MKSTEMP)
    IF(HAVE_MKSTEMP)
        ADD_DEFINITIONS(-DLUA_USE_MKSTEMP)
    ENDIF()
endif()

add_library(lua51 ${lua51_src})
