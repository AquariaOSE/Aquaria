# - Try to find TinyXML2
# Once done this will define
#  TINYXML2_FOUND - System has TinyXML2
#  TINYXML2_INCLUDE_DIRS - The TinyXML2 include directories
#  TINYXML2_LIBRARIES - The libraries needed to use TinyXML2
#  TINYXML2_DEFINITIONS - Compiler switches required for using TinyXML2

find_package(PkgConfig)
pkg_check_modules(PC_TINYXML2 QUIET tinyxml2)
set(TINYXML2_INCLUDE_DIRS ${PC_TINYXML2_INCLUDE_DIRS})
set(TINYXML2_DEFINITIONS ${PC_TINYXML2_CFLAGS_OTHER})

find_library(TINYXML2_LIBRARY NAMES tinyxml2
             HINTS ${PC_TINYXML2_LIBDIR} ${PC_TINYXML2_LIBRARY_DIRS})
set(TINYXML2_LIBRARIES ${TINYXML2_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TinyXML2 DEFAULT_MSG TINYXML2_LIBRARY TINYXML2_INCLUDE_DIRS)

mark_as_advanced(TINYXML2_INCLUDE_DIRS TINYXML2_LIBRARY)
