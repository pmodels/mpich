## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_NEMESIS_NETMOD_DCFA

lib_lib@MPILIBNAME@_la_SOURCES +=				\
    src/mpid/ch3/channels/nemesis/netmod/dcfa/dcfa_finalize.c	\
    src/mpid/ch3/channels/nemesis/netmod/dcfa/dcfa_init.c	\
    src/mpid/ch3/channels/nemesis/netmod/dcfa/dcfa_lmt.c	\
    src/mpid/ch3/channels/nemesis/netmod/dcfa/dcfa_poll.c	\
    src/mpid/ch3/channels/nemesis/netmod/dcfa/dcfa_reg_mr.c	\
    src/mpid/ch3/channels/nemesis/netmod/dcfa/dcfa_send.c	\
    src/mpid/ch3/channels/nemesis/netmod/dcfa/dcfa_ibcom.c 

noinst_HEADERS +=						\
    src/mpid/ch3/channels/nemesis/netmod/dcfa/dcfa_impl.h

endif BUILD_NEMESIS_NETMOD_DCFA
