# FindSWORD.cmake
# Find the SWORD Bible library
#
# This sets:
#   SWORD_FOUND        - True if SWORD was found
#   SWORD_INCLUDE_DIRS - SWORD include directories
#   SWORD_LIBRARIES    - SWORD libraries to link

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PC_SWORD QUIET sword)
endif()

find_path(SWORD_INCLUDE_DIRS
    NAMES swmgr.h
    PATH_SUFFIXES sword
    HINTS
        ${PC_SWORD_INCLUDEDIR}
        ${PC_SWORD_INCLUDE_DIRS}
        /usr/include
        /usr/local/include
)

find_library(SWORD_LIBRARIES
    NAMES sword
    HINTS
        ${PC_SWORD_LIBDIR}
        ${PC_SWORD_LIBRARY_DIRS}
        /usr/lib
        /usr/local/lib
        /usr/lib/x86_64-linux-gnu
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SWORD
    REQUIRED_VARS SWORD_LIBRARIES SWORD_INCLUDE_DIRS
)

mark_as_advanced(SWORD_INCLUDE_DIRS SWORD_LIBRARIES)
