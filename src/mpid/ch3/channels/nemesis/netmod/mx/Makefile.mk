## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2012 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_NEMESIS_NETMOD_MX

mpi_core_sources +=                                 \
    src/mpid/ch3/channels/nemesis/netmod/mx/mx_alloc.c    \
    src/mpid/ch3/channels/nemesis/netmod/mx/mx_cancel.c   \
    src/mpid/ch3/channels/nemesis/netmod/mx/mx_finalize.c \
    src/mpid/ch3/channels/nemesis/netmod/mx/mx_init.c     \
    src/mpid/ch3/channels/nemesis/netmod/mx/mx_poll.c     \
    src/mpid/ch3/channels/nemesis/netmod/mx/mx_probe.c    \
    src/mpid/ch3/channels/nemesis/netmod/mx/mx_send.c

noinst_HEADERS +=                                             \
    src/mpid/ch3/channels/nemesis/netmod/mx/mx_impl.h \
    src/mpid/ch3/channels/nemesis/netmod/mx/uthash.h

endif BUILD_NEMESIS_NETMOD_MX

