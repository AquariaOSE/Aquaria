
option(TTVFS_LARGEFILE_SUPPORT "Enable support for files > 4 GB? (experimental!)" FALSE)
option(TTVFS_IGNORE_CASE "Enable full case-insensitivity even on case-sensitive OSes like Linux and alike?" FALSE)

# Be sure to copy this part to your root CMakeLists.txt if you prefer to use CMake for configuring
# instead of editing the headers directly!
# If you edit the headers, this is not necessary.
if(TTVFS_LARGEFILE_SUPPORT)
    add_definitions("-DVFS_LARGEFILE_SUPPORT")
endif()
if(TTVFS_IGNORE_CASE)
    add_definitions("-DVFS_IGNORE_CASE")
endif()
# --snip--


# compiler specific things
if(MSVC)
    # MSVC builds require installed runtime library by default
    option(TTVFS_STATIC_LIB "Link as static library without runtime dependencies (Note: To get rid of this setting with MSVC, the cmake cache must be cleared)" FALSE)
    add_definitions("/GR-") # run-time type info (RTTI) not required
    
    if(TTVFS_STATIC_LIB)
        # this is ugly - hackfix compiler flags
        foreach(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            if(${flag_var} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            endif(${flag_var} MATCHES "/MD")
            if(${flag_var} MATCHES "/MDd")
                string(REGEX REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
            endif(${flag_var} MATCHES "/MDd")
        endforeach()
        
        # hackfix linker flags - no idea why, but MSVC will produce linker errors otherwise
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /NODEFAULTLIB")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:msvcrt.lib,msvcrtd.lib") # not sure if this is correct
        set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:msvcrt.lib,msvcrtd.lib")
    endif()
endif()


set(ttvfs_SRC
    VFS.h
    VFSAtomic.cpp
    VFSAtomic.h
    VFSDefines.h
    VFSDir.cpp
    VFSDir.h
    VFSFile.cpp
    VFSFile.h
    VFSHelper.cpp
    VFSHelper.h
    VFSInternal.h
    VFSLoader.cpp
    VFSLoader.h
    VFSSelfRefCounter.h
    VFSTools.cpp
    VFSTools.h
)

set(TTVFS_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "ttvfs include directory - for external includers" FORCE)
set(TTVFS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "ttvfs source directory - for external includers" FORCE)

add_library(ttvfs ${ttvfs_SRC})
