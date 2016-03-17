# We only execute this script if the user wants to use our bundled libjpeg.
#
# It is built as a static relocatable library, and linked into any target that
# requires it.

# This only has effect in this directory (lib/jpeg-9a).
include_directories(. include/)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

add_library(bundled_libjpeg STATIC include/jconfig.h
                                   include/jmorecfg.h
                                   include/jpegint.h
                                   include/jpeglib.h
                                   cderror.h
                                   jaricom.c
                                   jcapimin.c
                                   jcapistd.c
                                   jcarith.c
                                   jccoefct.c
                                   jccolor.c
                                   jcdctmgr.c
                                   jchuff.c
                                   jcinit.c
                                   jcmainct.c
                                   jcmarker.c
                                   jcmaster.c
                                   jcomapi.c
                                   jcparam.c
                                   jcprepct.c
                                   jcsample.c
                                   jctrans.c
                                   jdapimin.c
                                   jdapistd.c
                                   jdarith.c
                                   jdatadst.c
                                   jdatasrc.c
                                   jdcoefct.c
                                   jdcolor.c
                                   jdct.h
                                   jddctmgr.c
                                   jdhuff.c
                                   jdinput.c
                                   jdmainct.c
                                   jdmarker.c
                                   jdmaster.c
                                   jdmerge.c
                                   jdpostct.c
                                   jdsample.c
                                   jdtrans.c
                                   jerror.c
                                   jerror.h
                                   jfdctflt.c
                                   jfdctfst.c
                                   jfdctint.c
                                   jidctflt.c
                                   jidctfst.c
                                   jidctint.c
                                   jinclude.h
                                   jmemmgr.c
                                   jmemnobs.c
                                   jquant1.c
                                   jquant2.c
                                   jutils.c
                                   jversion.h
                                   rdbmp.c
                                   rdcolmap.c
                                   rdgif.c
                                   rdppm.c
                                   rdrle.c
                                   rdswitch.c
                                   rdtarga.c
                                   wrbmp.c
                                   wrgif.c
                                   wrppm.c
                                   wrrle.c
                                   wrtarga.c)

# Let consumers get at our bundled library in the standard CMake way.  These
# variables are not set in the cache, but instead shadow the variables in the
# cache.  (These are the same variables exported by the standard CMake FindJPEG
# module).
set(JPEG_LIBRARIES bundled_libjpeg PARENT_SCOPE)
set(JPEG_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/lib/jpeg-9a/include/ PARENT_SCOPE)

mark_as_advanced(JPEG_LIBRARIES JPEG_INCLUDE_DIR)

# Build as position-independent code, since it will probably be linked into a
# dynamic library.
set_property(TARGET bundled_libjpeg PROPERTY POSITION_INDEPENDENT_CODE True)
