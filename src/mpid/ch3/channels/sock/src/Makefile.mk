## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

# should be added unconditionally, since the error name list does not
# depend on any choices made by configure
errnames_txt_files += src/mpid/ch3/channels/sock/src/errnames.txt

if BUILD_CH3_SOCK

mpi_core_sources +=   \
    src/mpid/ch3/channels/sock/src/ch3_finalize.c    \
    src/mpid/ch3/channels/sock/src/ch3_init.c     \
    src/mpid/ch3/channels/sock/src/ch3_isend.c     \
    src/mpid/ch3/channels/sock/src/ch3_isendv.c    \
    src/mpid/ch3/channels/sock/src/ch3_istartmsg.c    \
    src/mpid/ch3/channels/sock/src/ch3_istartmsgv.c    \
    src/mpid/ch3/channels/sock/src/ch3_progress.c       \
    src/mpid/ch3/channels/sock/src/ch3_win_fns.c

endif BUILD_CH3_SOCK

