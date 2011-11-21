## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_NEMESIS_NETMOD_GM

lib_lib@MPILIBNAME@_la_SOURCES +=                                 \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/gm/gm_finalize.c \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/gm/gm_getput.c   \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/gm/gm_init.c     \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/gm/gm_lmt.c      \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/gm/gm_poll.c     \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/gm/gm_register.c \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/gm/gm_send.c     \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/gm/gm_test.c

noinst_HEADERS += src/mpid/ch3/channels/nemesis/nemesis/netmod/gm/gm_impl.h

endif BUILD_NEMESIS_NETMOD_GM

