# FindSWORD.cmake
# Find the SWORD Bible library
#
# This sets:
#   SWORD_FOUND        - True if SWORD was found
#   SWORD_INCLUDE_DIRS - SWORD include directories
#   SWORD_LIBRARIES    - SWORD libraries to link
#   SWORD_PREFIX       - Prefix where SWORD is installed

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    # On macOS and Windows we link against the static SWORD library, so request
    # STATIC mode so that pkg-config merges Libs.private (curl, z, bz2, lzma,
    # etc.) into the link list alongside the main sword library.
    if(WIN32 OR APPLE)
        pkg_check_modules(PC_SWORD QUIET STATIC sword)
    else()
        pkg_check_modules(PC_SWORD QUIET sword)
    endif()
endif()

find_path(SWORD_INCLUDE_DIRS
    NAMES swmgr.h
    PATH_SUFFIXES sword
    HINTS
        ${PC_SWORD_INCLUDEDIR}
        ${PC_SWORD_INCLUDE_DIRS}
        ${CMAKE_PREFIX_PATH}/include
        ENV SWORD_DIR
    PATHS
        /usr/include
        /usr/local/include
        /opt/homebrew/include
)

# On macOS/Windows, use the full static link library list from pkg-config when
# available (CMake 3.12+); this includes all transitive dependencies.
if((WIN32 OR APPLE) AND PC_SWORD_FOUND AND PC_SWORD_LINK_LIBRARIES)
    set(SWORD_LIBRARIES ${PC_SWORD_LINK_LIBRARIES})
else()
    if(WIN32 OR APPLE)
        set(_sword_names sword sword_static)
    else()
        set(_sword_names sword)
    endif()

    find_library(SWORD_LIBRARIES
        NAMES ${_sword_names}
        HINTS
            ${PC_SWORD_LIBDIR}
            ${PC_SWORD_LIBRARY_DIRS}
            ${CMAKE_PREFIX_PATH}/lib
            ENV SWORD_DIR
        PATHS
            /usr/lib
            /usr/local/lib
            /usr/lib/x86_64-linux-gnu
            /opt/homebrew/lib
    )
endif()

# Try to determine the SWORD prefix from the include path
if(SWORD_INCLUDE_DIRS)
    get_filename_component(_sword_inc_parent "${SWORD_INCLUDE_DIRS}" DIRECTORY)
    get_filename_component(SWORD_PREFIX "${_sword_inc_parent}" DIRECTORY)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SWORD
    REQUIRED_VARS SWORD_LIBRARIES SWORD_INCLUDE_DIRS
)

mark_as_advanced(SWORD_INCLUDE_DIRS SWORD_LIBRARIES SWORD_PREFIX)
