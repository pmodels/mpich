## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2012 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_NEMESIS_NETMOD_PORTALS4

mpi_core_sources +=					\
    src/mpid/ch3/channels/nemesis/netmod/portals4/ptl_init.c		\
    src/mpid/ch3/channels/nemesis/netmod/portals4/ptl_poll.c		\
    src/mpid/ch3/channels/nemesis/netmod/portals4/ptl_probe.c		\
    src/mpid/ch3/channels/nemesis/netmod/portals4/ptl_recv.c		\
    src/mpid/ch3/channels/nemesis/netmod/portals4/ptl_nm.c	        \
    src/mpid/ch3/channels/nemesis/netmod/portals4/ptl_send.c            \
    src/mpid/ch3/channels/nemesis/netmod/portals4/ptl_lmt.c             \
    src/mpid/ch3/channels/nemesis/netmod/portals4/rptl.c                \
    src/mpid/ch3/channels/nemesis/netmod/portals4/rptl_init.c           \
    src/mpid/ch3/channels/nemesis/netmod/portals4/rptl_op.c

noinst_HEADERS +=                                                \
    src/mpid/ch3/channels/nemesis/netmod/portals4/ptl_impl.h     \
    src/mpid/ch3/channels/nemesis/netmod/portals4/rptl.h

endif BUILD_NEMESIS_NETMOD_PORTALS4

