## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_CH3_NEMESIS

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch3/channels/nemesis/include   \
               -I$(top_builddir)/src/mpid/ch3/channels/nemesis/include

noinst_HEADERS +=                                             \
    src/mpid/ch3/channels/nemesis/include/mpidi_ch3_impl.h    \
    src/mpid/ch3/channels/nemesis/include/mpidi_ch3_nemesis.h \
    src/mpid/ch3/channels/nemesis/include/mpidi_ch3_post.h    \
    src/mpid/ch3/channels/nemesis/include/mpidi_ch3_pre.h

include $(top_srcdir)/src/mpid/ch3/channels/nemesis/src/Makefile.mk
include $(top_srcdir)/src/mpid/ch3/channels/nemesis/nemesis/Makefile.mk

endif BUILD_CH3_NEMESIS

