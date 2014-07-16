## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2014 Mellanox Technologies, Inc.
##     See COPYRIGHT in top-level directory.
##

if BUILD_NEMESIS_NETMOD_MXM

mpi_core_sources +=                                 		\
    src/mpid/ch3/channels/nemesis/netmod/mxm/mxm_cancel.c   \
    src/mpid/ch3/channels/nemesis/netmod/mxm/mxm_finalize.c \
    src/mpid/ch3/channels/nemesis/netmod/mxm/mxm_init.c     \
    src/mpid/ch3/channels/nemesis/netmod/mxm/mxm_poll.c    	\
    src/mpid/ch3/channels/nemesis/netmod/mxm/mxm_probe.c    \
    src/mpid/ch3/channels/nemesis/netmod/mxm/mxm_send.c

noinst_HEADERS +=                                           \
    src/mpid/ch3/channels/nemesis/netmod/mxm/mxm_impl.h

endif BUILD_NEMESIS_NETMOD_MXM

