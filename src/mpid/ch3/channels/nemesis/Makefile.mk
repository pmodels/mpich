## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_CH3_NEMESIS

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch3/channels/nemesis/include   \
               -I$(top_builddir)/src/mpid/ch3/channels/nemesis/include

noinst_HEADERS +=							\
    src/mpid/ch3/channels/nemesis/include/mpidi_ch3_impl.h		\
    src/mpid/ch3/channels/nemesis/include/mpidi_ch3_nemesis.h		\
    src/mpid/ch3/channels/nemesis/include/mpidi_ch3_post.h		\
    src/mpid/ch3/channels/nemesis/include/mpidi_ch3_pre.h		\
    src/mpid/ch3/channels/nemesis/include/mpid_nem_atomics.h		\
    src/mpid/ch3/channels/nemesis/include/mpid_nem_datatypes.h		\
    src/mpid/ch3/channels/nemesis/include/mpid_nem_debug.h		\
    src/mpid/ch3/channels/nemesis/include/mpid_nem_defs.h		\
    src/mpid/ch3/channels/nemesis/include/mpid_nem_fbox.h		\
    src/mpid/ch3/channels/nemesis/include/mpid_nem_generic_queue.h	\
    src/mpid/ch3/channels/nemesis/include/mpid_nem_impl.h		\
    src/mpid/ch3/channels/nemesis/include/mpid_nem_inline.h		\
    src/mpid/ch3/channels/nemesis/include/mpid_nem_nets.h		\
    src/mpid/ch3/channels/nemesis/include/mpid_nem_post.h		\
    src/mpid/ch3/channels/nemesis/include/mpid_nem_pre.h		\
    src/mpid/ch3/channels/nemesis/include/mpid_nem_queue.h

include $(top_srcdir)/src/mpid/ch3/channels/nemesis/src/Makefile.mk
include $(top_srcdir)/src/mpid/ch3/channels/nemesis/netmod/Makefile.mk
include $(top_srcdir)/src/mpid/ch3/channels/nemesis/utils/monitor/Makefile.mk
include $(top_srcdir)/src/mpid/ch3/channels/nemesis/utils/replacements/Makefile.mk

endif BUILD_CH3_NEMESIS

