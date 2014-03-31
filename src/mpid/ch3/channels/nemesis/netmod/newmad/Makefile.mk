## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2012 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_NEMESIS_NETMOD_NEWMAD

mpi_core_sources +=                                 \
    src/mpid/ch3/channels/nemesis/netmod/newmad/newmad_finalize.c \
    src/mpid/ch3/channels/nemesis/netmod/newmad/newmad_init.c     \
    src/mpid/ch3/channels/nemesis/netmod/newmad/newmad_poll.c     \
    src/mpid/ch3/channels/nemesis/netmod/newmad/newmad_send.c     \
    src/mpid/ch3/channels/nemesis/netmod/newmad/newmad_register.c \
    src/mpid/ch3/channels/nemesis/netmod/newmad/newmad_test.c     \
    src/mpid/ch3/channels/nemesis/netmod/newmad/newmad_cancel.c   \
    src/mpid/ch3/channels/nemesis/netmod/newmad/newmad_probe.c    \
    src/mpid/ch3/channels/nemesis/netmod/newmad/newmad_alloc.c

noinst_HEADERS +=                                                           \
    src/mpid/ch3/channels/nemesis/netmod/newmad/newmad_impl.h               \
    src/mpid/ch3/channels/nemesis/netmod/newmad/newmad_extended_interface.h

endif BUILD_NEMESIS_NETMOD_NEWMAD

