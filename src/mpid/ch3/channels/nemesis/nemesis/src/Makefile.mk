## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

lib_lib@MPILIBNAME@_la_SOURCES +=                                         \
    src/mpid/ch3/channels/nemesis/nemesis/src/mpid_nem_alloc.c        \
    src/mpid/ch3/channels/nemesis/nemesis/src/mpid_nem_init.c         \
    src/mpid/ch3/channels/nemesis/nemesis/src/mpid_nem_queue.c        \
    src/mpid/ch3/channels/nemesis/nemesis/src/mpid_nem_barrier.c      \
    src/mpid/ch3/channels/nemesis/nemesis/src/mpid_nem_mpich2.c       \
    src/mpid/ch3/channels/nemesis/nemesis/src/mpid_nem_ckpt.c         \
    src/mpid/ch3/channels/nemesis/nemesis/src/mpid_nem_finalize.c     \
    src/mpid/ch3/channels/nemesis/nemesis/src/mpid_nem_network_poll.c \
    src/mpid/ch3/channels/nemesis/nemesis/src/mpid_nem_network.c      \
    src/mpid/ch3/channels/nemesis/nemesis/src/mpid_nem_debug.c        \
    src/mpid/ch3/channels/nemesis/nemesis/src/mpid_nem_lmt.c          \
    src/mpid/ch3/channels/nemesis/nemesis/src/mpid_nem_lmt_shm.c      \
    src/mpid/ch3/channels/nemesis/nemesis/src/mpid_nem_net_array.c    \
    src/mpid/ch3/channels/nemesis/nemesis/src/mpid_nem_lmt_dma.c      \
    src/mpid/ch3/channels/nemesis/nemesis/src/mpid_nem_lmt_vmsplice.c

