## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_NEMESIS_NETMOD_PORTALS4

lib_lib@MPILIBNAME@_la_SOURCES +=					\
    src/mpid/ch3/channels/nemesis/netmod/portals4/ptl_init.c		\
    src/mpid/ch3/channels/nemesis/netmod/portals4/ptl_pack_byte.c	\
    src/mpid/ch3/channels/nemesis/netmod/portals4/ptl_poll.c		\
    src/mpid/ch3/channels/nemesis/netmod/portals4/ptl_probe.c		\
    src/mpid/ch3/channels/nemesis/netmod/portals4/ptl_recv.c		\
    src/mpid/ch3/channels/nemesis/netmod/portals4/ptl_nm.c	        \
    src/mpid/ch3/channels/nemesis/netmod/portals4/ptl_send.c 

noinst_HEADERS +=                                                \
    src/mpid/ch3/channels/nemesis/netmod/portals4/ptl_impl.h 

endif BUILD_NEMESIS_NETMOD_PORTALS4

