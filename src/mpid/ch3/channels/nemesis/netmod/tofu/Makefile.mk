## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_NEMESIS_NETMOD_TOFU

# AM_CPPFLAGS += -I...

lib_lib@MPILIBNAME@_la_SOURCES +=					\
	src/mpid/ch3/channels/nemesis/netmod/tofu/tofu_init.c		\
	src/mpid/ch3/channels/nemesis/netmod/tofu/tofu_fini.c		\
	src/mpid/ch3/channels/nemesis/netmod/tofu/tofu_vc.c		\
	src/mpid/ch3/channels/nemesis/netmod/tofu/tofu_poll.c		\
	src/mpid/ch3/channels/nemesis/netmod/tofu/tofu_send.c		\
	src/mpid/ch3/channels/nemesis/netmod/tofu/tofu_probe.c		\
	src/mpid/ch3/channels/nemesis/netmod/tofu/tofu_cancel.c		\
	$(EOA)

noinst_HEADERS +=							\
	src/mpid/ch3/channels/nemesis/netmod/tofu/tofu_impl.h		\
	$(EOA)

endif BUILD_NEMESIS_NETMOD_TOFU

