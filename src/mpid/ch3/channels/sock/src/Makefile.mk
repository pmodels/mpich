## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

# this file is externally guarded by "if BUILD_CH3_SOCK"

# FIXME this current file does not handle the dllchan helper library build
# (libmpich2-ch3-sock.so, I think).
#
# Should we do any work to accommodate it?  If so, should we use the existing
# hacked-up mechanisms or should we be using ltdl and/or libtool helper
# libraries?

lib_lib@MPILIBNAME@_la_SOURCES +=   \
    src/mpid/ch3/channels/sock/src/ch3_finalize.c    \
    src/mpid/ch3/channels/sock/src/ch3_init.c     \
    src/mpid/ch3/channels/sock/src/ch3_isend.c     \
    src/mpid/ch3/channels/sock/src/ch3_isendv.c    \
    src/mpid/ch3/channels/sock/src/ch3_istartmsg.c    \
    src/mpid/ch3/channels/sock/src/ch3_istartmsgv.c    \
    src/mpid/ch3/channels/sock/src/ch3_progress.c

errnames_txt_files += src/mpid/ch3/channels/sock/src/errnames.txt

