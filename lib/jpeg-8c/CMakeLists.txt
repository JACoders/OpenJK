# We only execute this script if the user wants to use our bundled libjpeg.
#
# It is built as a static relocatable library, and linked into any target that
# requires it.

# This only has effect in this directory (lib/jpeg-8c).
include_directories(. include/)

add_library(bundled_libjpeg STATIC include/jpeglib.h
                                   include/jconfig.h
                                   include/jmorecfg.h
                                   include/jpegint.h
                                   jcparam.c
                                   jdsample.c
                                   jerror.c
                                   jdapistd.c
                                   jcarith.c
                                   jdcoefct.c
                                   jmemmgr.c
                                   jfdctfst.c
                                   jdhuff.c
                                   jerror.h
                                   jutils.c
                                   jdmerge.c
                                   jinclude.h
                                   jcprepct.c
                                   jidctfst.c
                                   jdatadst.c
                                   jidctint.c
                                   jversion.h
                                   jccolor.c
                                   jmemsys.h
                                   jfdctint.c
                                   jdmaster.c
                                   jcdctmgr.c
                                   jdct.h
                                   jdinput.c
                                   jdatasrc.c
                                   jdpostct.c
                                   jcsample.c
                                   jchuff.c
                                   jddctmgr.c
                                   jdapimin.c
                                   jcapimin.c
                                   jdcolor.c
                                   jcmainct.c
                                   jfdctflt.c
                                   jctrans.c
                                   jquant1.c
                                   jccoefct.c
                                   jdmainct.c
                                   jcinit.c
                                   jidctflt.c
                                   jdmarker.c
                                   jdtrans.c
                                   jdarith.c
                                   jaricom.c
                                   jcomapi.c
                                   jcmarker.c
                                   jmemnobs.c
                                   jcmaster.c
                                   jcapistd.c
                                   jquant2.c)

# Let consumers get at our bundled library in the standard CMake way.  These
# variables are not set in the cache, but instead shadow the variables in the
# cache.  (These are the same variables exported by the standard CMake FindJPEG
# module).
set(JPEG_LIBRARIES bundled_libjpeg PARENT_SCOPE)
set(JPEG_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/lib/jpeg-8c/include/ PARENT_SCOPE)

mark_as_advanced(JPEG_LIBRARIES JPEG_INCLUDE_DIR)

# Build as position-independent code, since it will probably be linked into a
# dynamic library.
set_property(TARGET bundled_libjpeg PROPERTY POSITION_INDEPENDENT_CODE True)
