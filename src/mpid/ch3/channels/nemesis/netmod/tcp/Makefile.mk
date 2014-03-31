## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2012 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_NEMESIS_NETMOD_TCP

mpi_core_sources +=                                   \
    src/mpid/ch3/channels/nemesis/netmod/tcp/tcp_finalize.c \
    src/mpid/ch3/channels/nemesis/netmod/tcp/tcp_init.c     \
    src/mpid/ch3/channels/nemesis/netmod/tcp/tcp_send.c     \
    src/mpid/ch3/channels/nemesis/netmod/tcp/tcp_utility.c  \
    src/mpid/ch3/channels/nemesis/netmod/tcp/socksm.c       \
    src/mpid/ch3/channels/nemesis/netmod/tcp/tcp_getip.c    \
    src/mpid/ch3/channels/nemesis/netmod/tcp/tcp_ckpt.c

noinst_HEADERS +=                                                \
    src/mpid/ch3/channels/nemesis/netmod/tcp/socksm.h    \
    src/mpid/ch3/channels/nemesis/netmod/tcp/tcp_impl.h  \
    src/mpid/ch3/channels/nemesis/netmod/tcp/tcp_queue.h

endif BUILD_NEMESIS_NETMOD_TCP

