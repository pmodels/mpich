## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

include $(top_srcdir)/src/mpid/ch3/channels/sock/src/Makefile.mk

if BUILD_CH3_SOCK

AM_CPPFLAGS +=                                           \
    -I$(top_srcdir)/src/mpid/ch3/channels/sock/include   \
    -I$(top_builddir)/src/mpid/ch3/channels/sock/include

noinst_HEADERS +=                                       \
    src/mpid/ch3/channels/sock/include/mpidi_ch3_pre.h  \
    src/mpid/ch3/channels/sock/include/mpidi_ch3_impl.h \
    src/mpid/ch3/channels/sock/include/mpidi_ch3_post.h

endif BUILD_CH3_SOCK

