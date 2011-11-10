## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

lib_lib@MPILIBNAME@_la_SOURCES +=                           \
    src/mpid/ch3/channels/nemesis/src/ch3_finalize.c        \
    src/mpid/ch3/channels/nemesis/src/ch3_init.c            \
    src/mpid/ch3/channels/nemesis/src/ch3_isend.c           \
    src/mpid/ch3/channels/nemesis/src/ch3_isendv.c          \
    src/mpid/ch3/channels/nemesis/src/ch3_istartmsg.c       \
    src/mpid/ch3/channels/nemesis/src/ch3_istartmsgv.c      \
    src/mpid/ch3/channels/nemesis/src/ch3_progress.c        \
    src/mpid/ch3/channels/nemesis/src/ch3_abort.c           \
    src/mpid/ch3/channels/nemesis/src/ch3i_comm.c           \
    src/mpid/ch3/channels/nemesis/src/ch3i_eagernoncontig.c

