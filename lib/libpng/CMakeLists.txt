# We only execute this script if the user wants to use our bundled libpng.
#
# It is built as a static relocatable library, and linked into any target that
# requires it.

# This only has effect in this directory (lib/png).
include_directories(include/)
if(WIN32)
	include_directories(../zlib/include/)
endif()

add_library(bundled_libpng STATIC include/png.h
                                  include/pngconf.h
                                  include/pnglibconf.h
                                  png.c
                                  pngdebug.h
                                  pngerror.c
                                  pngget.c
                                  pnginfo.h
                                  pngmem.c
                                  pngpread.c
                                  pngpriv.h
                                  pngread.c
                                  pngrio.c
                                  pngrtran.c
                                  pngrutil.c
                                  pngset.c
                                  pngstruct.h
                                  pngtrans.c
                                  pngwio.c
                                  pngwrite.c
                                  pngwtran.c
                                  pngwutil.c)

# Let consumers get at our bundled library in the standard CMake way.  These
# variables are not set in the cache, but instead shadow the variables in the
# cache.  (These are the same variables exported by the standard CMake FindPNG
# module).
set(PNG_LIBRARIES bundled_libpng PARENT_SCOPE)
set(PNG_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/lib/libpng/include/ PARENT_SCOPE)

mark_as_advanced(PNG_LIBRARIES PNG_INCLUDE_DIRS)

# Build as position-independent code, since it will probably be linked into a
# dynamic library.
set_property(TARGET bundled_libpng PROPERTY POSITION_INDEPENDENT_CODE True)
