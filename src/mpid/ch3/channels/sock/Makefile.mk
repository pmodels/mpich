##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
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
