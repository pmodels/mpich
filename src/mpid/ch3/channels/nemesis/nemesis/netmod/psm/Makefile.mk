## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_NEMESIS_NETMOD_PSM

lib_lib@MPILIBNAME@_la_SOURCES +=                                   \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/psm/psm_finalize.c \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/psm/psm_init.c     \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/psm/psm_poll.c     \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/psm/psm_send.c     \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/psm/psm_getput.c   \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/psm/psm_lmt.c      \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/psm/psm_register.c \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/psm/psm_test.c

noinst_HEADERS += src/mpid/ch3/channels/nemesis/nemesis/netmod/psm/psm_impl.h

endif BUILD_NEMESIS_NETMOD_PSM

