## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_NEMESIS_NETMOD_PORTALS4NM

lib_lib@MPILIBNAME@_la_SOURCES +=                                   \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/portals4nm/ptlnm_init.c \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/portals4nm/ptlnm_poll.c \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/portals4nm/ptlnm_send.c 

noinst_HEADERS +=                                                \
    src/mpid/ch3/channels/nemesis/nemesis/netmod/portals4nm/ptlnm_impl.h 

endif BUILD_NEMESIS_NETMOD_PORTALS4NM

