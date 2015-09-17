# - Try to find FTGL
# Once done this will define
#  FTGL_FOUND - System has FTGL
#  FTGL_INCLUDE_DIRS - The FTGL include directories
#  FTGL_LIBRARIES - The libraries needed to use FTGL
#  FTGL_DEFINITIONS - Compiler switches required for using FTGL

find_package(PkgConfig)
pkg_check_modules(PC_FTGL QUIET ftgl)
set(FTGL_INCLUDE_DIRS ${PC_FTGL_INCLUDE_DIRS})
set(FTGL_DEFINITIONS ${PC_FTGL_CFLAGS_OTHER})

find_library(FTGL_LIBRARY ftgl
             HINTS ${PC_FTGL_LIBDIR} ${PC_FTGL_LIBRARY_DIRS})
set(FTGL_LIBRARIES ${FTGL_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FTGL DEFAULT_MSG FTGL_LIBRARIES FTGL_INCLUDE_DIRS)

mark_as_advanced(FTGL_LIBRARIES FTGL_INCLUDE_DIRS)
