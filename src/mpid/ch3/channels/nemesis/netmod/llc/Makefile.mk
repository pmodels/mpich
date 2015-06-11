## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_NEMESIS_NETMOD_LLC

# AM_CPPFLAGS += -I...

lib_lib@MPILIBNAME@_la_SOURCES +=					\
	src/mpid/ch3/channels/nemesis/netmod/llc/llc_init.c		\
	src/mpid/ch3/channels/nemesis/netmod/llc/llc_fini.c		\
	src/mpid/ch3/channels/nemesis/netmod/llc/llc_vc.c		\
	src/mpid/ch3/channels/nemesis/netmod/llc/llc_poll.c		\
	src/mpid/ch3/channels/nemesis/netmod/llc/llc_send.c		\
	src/mpid/ch3/channels/nemesis/netmod/llc/llc_probe.c		\
	src/mpid/ch3/channels/nemesis/netmod/llc/llc_cancel.c		\
	$(EOA)

noinst_HEADERS +=							\
	src/mpid/ch3/channels/nemesis/netmod/llc/llc_impl.h		\
	$(EOA)

endif BUILD_NEMESIS_NETMOD_LLC

