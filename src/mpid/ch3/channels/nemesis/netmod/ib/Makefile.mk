## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_NEMESIS_NETMOD_IB

lib_lib@MPILIBNAME@_la_SOURCES +=				\
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_finalize.c	\
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_init.c	\
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_lmt.c	\
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_poll.c	\
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_reg_mr.c	\
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_send.c	\
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_ibcom.c 

noinst_HEADERS +=						\
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_impl.h           \
    src/mpid/ch3/channels/nemesis/netmod/ib/ib_ibcom.h

endif BUILD_NEMESIS_NETMOD_IB
