## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

lib_lib@MPILIBNAME@_la_SOURCES +=                                       \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/tcp/tcp_finalize.c \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/tcp/tcp_init.c     \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/tcp/tcp_send.c     \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/tcp/tcp_utility.c  \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/tcp/socksm.c       \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/tcp/tcp_getip.c    \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/tcp/tcp_ckpt.c

noinst_HEADERS +=                                                \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/tcp/socksm.h    \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/tcp/tcp_impl.h  \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/tcp/tcp_queue.h

