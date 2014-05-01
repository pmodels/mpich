## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2013 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_NEMESIS_NETMOD_IB

mpi_core_sources +=				\
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_finalize.c	\
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_init.c	\
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_lmt.c	\
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_poll.c	\
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_reg_mr.c	\
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_send.c	\
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_ibcom.c	\
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_malloc.c

noinst_HEADERS +=						\
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_impl.h           \
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_ibcom.h

endif BUILD_NEMESIS_NETMOD_IB
