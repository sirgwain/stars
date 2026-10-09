set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# MSYS2's native tools have unprefixed names; cross toolchains use the
# target triple. On Windows, put the desired 64-bit MinGW bin directory
# (for example C:/msys64/ucrt64/bin) first on PATH.
if(CMAKE_HOST_WIN32)
    set(STARS_MINGW_PREFIX "")
else()
    set(STARS_MINGW_PREFIX "x86_64-w64-mingw32-")
endif()
set(CMAKE_C_COMPILER ${STARS_MINGW_PREFIX}gcc)
set(CMAKE_RC_COMPILER ${STARS_MINGW_PREFIX}windres)
set(CMAKE_AR ${STARS_MINGW_PREFIX}ar)
set(CMAKE_RANLIB ${STARS_MINGW_PREFIX}ranlib)
set(CMAKE_NM ${STARS_MINGW_PREFIX}nm)
set(CMAKE_STRIP ${STARS_MINGW_PREFIX}strip)

# Build tools run on the host; headers, libraries, and packages target Windows.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
